#include "../algorithms/kpkc.h"
#include "clique_bron_kerbosch.h"
#include "clique_help_functions.h"
#include "clique_successor_generator.h"

#include "../action.h"
#include "../database/hash_join.h"
#include "../database/semi_join.h"
#include "../database/table.h"
#include "../task.h"

#include <cassert>
#include <limits>
#include <unordered_map>
#include <vector>

#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <unordered_set>
#include <vector>

using namespace std;

bool CliqueSuccessorGenerator::consistent_literals(const vector<Atom> &literals,
                                                   const Assignment &first_assignment,
                                                   const Assignment &second_assignment) {
    const auto num_objects = task.objects.size();

    for (const auto &literal : literals) {
        int first_position = -1;
        int second_position = -1;
        int first_object = -1;
        int second_object = -1;
        bool empty_assignment = true;
        const auto &parameters = literal.get_arguments();
        const auto predicate_arity = parameters.size();

        // For literals of arity > 2 that contains constants,
        // the constants are ignored which means that we might
        // enumerate inapplicable ground actions.
        // (This is not a problem since we test such predicates.)

        for (size_t index = 0; index < predicate_arity; ++index) {
            const auto &parameter = parameters[index];
            const auto is_constant = parameter.is_constant();
            const auto parameter_index = parameter.get_index();

            if (is_constant) {
                if (predicate_arity <= 2) {
                    if (first_position < 0) {
                        first_position = index;
                        first_object = parameter_index;
                    }
                    else {
                        second_position = index;
                        second_object = parameter_index;
                    }

                    empty_assignment = false;
                }
            }
            else {
                if (first_assignment.parameter_index == parameter_index) {
                    if (first_position < 0) {
                        first_position = index;
                        first_object = first_assignment.object_index;
                    }
                    else {
                        second_position = index;
                        second_object = first_assignment.object_index;
                        break;
                    }

                    empty_assignment = false;
                }
                else if (second_assignment.parameter_index == parameter_index) {
                    if (first_position < 0) {
                        first_position = index;
                        first_object = second_assignment.object_index;
                    }
                    else {
                        second_position = index;
                        second_object = second_assignment.object_index;
                        break;
                    }

                    empty_assignment = false;
                }
            }
        }

        if (!empty_assignment) {
            // The size of assignment_sets should be number of predicates,
            // and each assignment_set should have size determined by total_ranks(.).
            // That is, the indexing should be safe.

            const auto predicate_symbol = literal.get_predicate_symbol_idx();
            const auto &assignment_set = assignment_sets[predicate_symbol];
            const auto assignment_rank = get_rank(first_position,
                                                  first_object,
                                                  second_position,
                                                  second_object,
                                                  predicate_arity,
                                                  num_objects);
            const auto consistent_with_state = assignment_set[assignment_rank];

            // A positive literal is inconsistent if there does not exist a (partially ground) atom
            // in the state. A negative literal is inconsistent if the assignment matches a fully
            // ground atom in the state.

            if (!literal.is_negated() && !consistent_with_state) {
                return false;
            }
            else if (literal.is_negated() && consistent_with_state &&
                     ((predicate_arity == 1) ||
                      ((predicate_arity == 2) && (second_position >= 0)))) {
                return false;
            }
        }
    }

    return true;
}

bool CliqueSuccessorGenerator::consistent_literals_with_constants(const vector<Atom> &literals) {
    const auto empty_assignment = Assignment(-1, -1);
    return consistent_literals(literals, empty_assignment, empty_assignment);
}


bool CliqueSuccessorGenerator::test_nullary_preconditions(const ActionSchema &action,
                                                          const DBState &state) {
    // If a nullary atom does not hold then all instantiations are inapplicable.
    const auto &nullary_atoms = state.get_nullary_atoms();
    const auto &positive_nullary = action.get_positive_nullary_precond();
    for (uint32_t index = 0; index < positive_nullary.size(); ++index) {
        if (positive_nullary[index] && !nullary_atoms[index]) {
            return false;
        }
    }

    const auto &negative_nullary = action.get_negative_nullary_precond();
    for (uint32_t index = 0; index < negative_nullary.size(); ++index) {
        if (negative_nullary[index] && nullary_atoms[index]) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Builds a datastructure to efficiently test if an assignment is compatible with an atom in
 * the given set.
 *
 * @return A set for each predicate that contains possible compatible assignments.
 */
void CliqueSuccessorGenerator::build_assignment_sets(const DBState &atoms) {
    const auto num_objects = task.objects.size();
    const auto &relations = atoms.get_relations();

    if (assignment_sets.size() < relations.size()) {
        assignment_sets.resize(relations.size());
    }

    for (auto &assignment_set : assignment_sets) {
        assignment_set.clear();
    }

    for (const auto &relation : relations) {
        const auto predicate_arity = task.predicates[relation.predicate_symbol].getArity();
        auto &assignment_set = assignment_sets[relation.predicate_symbol];
        assignment_set.resize(total_ranks(predicate_arity, num_objects));

        for (const auto &atom : relation.tuples) {
            for (size_t first_position = 0; first_position < atom.size(); ++first_position) {
                const auto first_object = atom[first_position];

                assignment_set[get_rank(
                    first_position, first_object, -1, -1, predicate_arity, num_objects)] = true;

                for (size_t second_position = first_position + 1; second_position < atom.size();
                     ++second_position) {
                    const auto second_object = atom[second_position];
                    assignment_set[get_rank(
                        second_position, second_object, -1, -1, predicate_arity, num_objects)] =
                        true;
                    assignment_set[get_rank(first_position,
                                            first_object,
                                            second_position,
                                            second_object,
                                            predicate_arity,
                                            num_objects)] = true;
                }
            }
        }
    }
}

CliqueSuccessorGenerator::CliqueSuccessorGenerator(const Task &task, const CliquePivot &pivot)
    : GenericJoinSuccessor(task), task(task), pivot(pivot) {
    // Types are static information, precompute the set of object that
    // can be assigned to each parameter based on type information.

    // Inequality is translated to static binary relations, we do not
    // need to take special care of these.

    build_assignment_sets(task.get_static_info());

    for (const auto &action : task.get_action_schemas()) {
        unordered_map<Parameter, set<Object>> parameter_assignments;
        for (const auto &parameter : action.get_parameters()) {
            set<Object> compatible_objects;

            for (const auto &object : task.objects) {
                for (const auto &type : object.get_types()) {
                    if (type == parameter.type) {
                        compatible_objects.insert(object);
                    }
                }
            }

            parameter_assignments.insert(make_pair(parameter, compatible_objects));
        }

        action_objects_by_parameter_type.insert(make_pair(action, parameter_assignments));

        int last_cumulative_partition_size = 0;
        vector<vector<size_t>> partitions;
        vector<Assignment> to_vertex_assignment;

        for (const auto &parameter : action.get_parameters()) {
            vector<size_t> partition;
            const auto compatible_objects = parameter_assignments.at(parameter);
            const int cumulative_partition_size =
                last_cumulative_partition_size + (int)compatible_objects.size();
            last_cumulative_partition_size = cumulative_partition_size;

            for (const auto &object : compatible_objects) {
                partition.push_back(to_vertex_assignment.size());
                const Assignment vertex_assignment(parameter.get_index(), object.get_index());
                to_vertex_assignment.push_back(vertex_assignment);
            }

            partitions.push_back(std::move(partition));
        }

        set<string> static_predicate_names;
        set<string> dynamic_predicate_names;

        for (const auto &predicate : task.predicates) {
            if (predicate.get_name().rfind("type@", 0) == string::npos) {
                if (predicate.isStaticPredicate()) {
                    static_predicate_names.insert(predicate.get_name());
                }
                else {
                    dynamic_predicate_names.insert(predicate.get_name());
                }
            }
        }

        const auto &literals = action.get_precondition();
        vector<Atom> static_precondition;
        vector<Atom> dynamic_precondition;
        vector<AssignmentPair> statically_consistent_assignments;

        for (const auto &literal : literals) {
            if (static_predicate_names.find(literal.get_name()) != static_predicate_names.end()) {
                static_precondition.push_back(literal);
            }
            else if (dynamic_predicate_names.find(literal.get_name()) !=
                     dynamic_predicate_names.end()) {
                dynamic_precondition.push_back(literal);
            }
        }

        for (size_t first_id = 0; first_id < to_vertex_assignment.size(); ++first_id) {
            for (size_t second_id = (first_id + 1); second_id < to_vertex_assignment.size();
                 ++second_id) {
                const auto &first_assignment = to_vertex_assignment.at(first_id);
                const auto &second_assignment = to_vertex_assignment.at(second_id);

                if (first_assignment.parameter_index != second_assignment.parameter_index) {
                    if (consistent_literals(
                            static_precondition, first_assignment, second_assignment)) {
                        statically_consistent_assignments.push_back(AssignmentPair(
                            first_id, first_assignment, second_id, second_assignment));
                    }
                }
            }
        }

        action_precondition.insert(
            make_pair(action, Precondition(static_precondition, dynamic_precondition)));
        action_statically_consistent_assignments.insert(
            make_pair(action, statically_consistent_assignments));
        action_to_vertex_assignment.insert(make_pair(action, to_vertex_assignment));
        action_partitions.insert(make_pair(action, partitions));
    }
}

vector<LiftedOperatorId> CliqueSuccessorGenerator::applicable_actions_nullary_case(const ActionSchema &action,
                                                                                   const DBState &state) {
    vector<LiftedOperatorId> applicable_actions;

    const auto &preconds = action_precondition.find(action)->second;
    const auto &static_preconds = preconds.static_preconds;
    const auto &dynamic_preconds = preconds.dynamic_preconds;
    const auto &static_info = task.get_static_info();
    const LiftedOperatorId op(action.get_index(), GroundAtom(), unordered_map<int, int>());

    if (literal_holds(op, dynamic_preconds, static_preconds, state, static_info, 1)) {
        applicable_actions.push_back(op);
    }

    return applicable_actions;
}

vector<LiftedOperatorId> CliqueSuccessorGenerator::applicable_actions_unary_case(const ActionSchema &action,
                                                                                 const DBState &state) {
    vector<LiftedOperatorId> applicable_actions;

    const auto &preconds = action_precondition.find(action)->second;
    const auto &static_preconds = preconds.static_preconds;
    const auto &dynamic_preconds = preconds.dynamic_preconds;
    const auto &static_info = task.get_static_info();
    const auto &objects_by_parameter_type = action_objects_by_parameter_type.find(action)->second;
    const auto &action_parameters = action.get_parameters();
    const auto &parameter = action_parameters.at(0);
    const auto &objects_with_correct_type = objects_by_parameter_type.at(parameter);

    for (const auto &object : objects_with_correct_type) {
        const LiftedOperatorId op(action.get_index(), {object.get_index()}, unordered_map<int, int>());

        if (literal_holds(op, dynamic_preconds, static_preconds, state, static_info, 1)) {
            applicable_actions.push_back(op);
        }
    }

    return applicable_actions;
}

vector<uint32_t> CliqueSuccessorGenerator::create_adjacency_list(const Precondition &preconds,
                                                                 const std::vector<AssignmentPair> &statically_consistent_assignments,
                                                                 std::size_t num_parameters,
                                                                 vector<uint32_t> *adjacency_list,
                                                                 std::size_t num_vertices) {
    for (uint32_t vertex_id = 0; vertex_id < num_vertices; ++vertex_id) {
        adjacency_list[vertex_id] = vector<uint32_t>();
    }

    for (const auto &pair : statically_consistent_assignments) {
        if (consistent_literals(
                preconds.dynamic_preconds, pair.first_assignment, pair.second_assignment)) {
            auto &first_neighbors = adjacency_list[pair.first_position];
            auto &second_neighbors = adjacency_list[pair.second_position];
            first_neighbors.push_back(pair.second_position);
            second_neighbors.push_back(pair.first_position);
        }
    }

    // Make the graph more sparse by removing edges between vertices
    // that has a degree less than num_parameters -1. We can safely
    // do so since they cannot be part of any maximum clique, i.e.,
    // complete assignment of the parameters.

    bool removed_edges = true;
    while (removed_edges) {
        removed_edges = false;

        for (uint32_t id = 0; id < num_vertices; ++id) {
            auto &neighbors = adjacency_list[id];
            const auto num_neighbors = neighbors.size();

            if ((num_neighbors > 0) && (num_neighbors < (num_parameters - 1))) {
                for (const auto neighbor_id : neighbors) {
                    auto &neighbors = adjacency_list[neighbor_id];
                    neighbors.erase(find(neighbors.begin(), neighbors.end(), id));
                }

                neighbors.clear();
                removed_edges = true;
            }
        }
    }

    vector<uint32_t> vertex_ids;
    for (uint32_t id = 0; id < num_vertices; ++id) {
        auto &vertices = adjacency_list[id];
        if (vertices.size() > 0) {
            vertex_ids.push_back(id);
            sort(vertices.begin(), vertices.end());
        }
    }

    return vertex_ids;
}

void CliqueSuccessorGenerator::create_adjacency_matrix(const Precondition &preconds,
                                                       const std::vector<AssignmentPair> &statically_consistent_assignments,
                                                       std::size_t num_parameters,
                                                       std::size_t num_vertices,
                                                       std::vector<boost::dynamic_bitset<>> &adjacency_matrix) {
    for (const auto &pair : statically_consistent_assignments) {
        if (consistent_literals(
                preconds.dynamic_preconds, pair.first_assignment, pair.second_assignment)) {
            auto &first_row = adjacency_matrix[pair.first_position];
            auto &second_row = adjacency_matrix[pair.second_position];
            first_row[pair.second_position] = 1;
            second_row[pair.first_position] = 1;
        }
    }
}

vector<LiftedOperatorId> CliqueSuccessorGenerator::applicable_actions_general_case(const ActionSchema &action,
                                                                                   const DBState &state) {
    vector<LiftedOperatorId> applicable_actions;

    const auto &preconds = action_precondition.find(action)->second;

    if (!consistent_literals_with_constants(preconds.dynamic_preconds_with_constants)) {
        return applicable_actions;
    }

    const auto &action_parameters = action.get_parameters();
    const auto num_parameters = action_parameters.size();

    const auto &to_vertex_assignment = action_to_vertex_assignment.find(action)->second;
    const auto &statically_consistent_assignments =
        action_statically_consistent_assignments.find(action)->second;

    vector<vector<uint32_t>> cliques;

    if (num_parameters == 2) {
        for (const auto &pair : statically_consistent_assignments) {
            if (consistent_literals(
                    preconds.dynamic_preconds, pair.first_assignment, pair.second_assignment)) {
                cliques.push_back(std::vector<uint32_t>(
                    {(uint32_t)pair.first_position, (uint32_t)pair.second_position}));
            }
        }
    }
    else {
        switch (pivot) {
        case BronKerboschFirst: {
            const auto num_vertices = to_vertex_assignment.size();
            vector<uint32_t> adjacency_list[num_vertices];
            const auto vertex_ids = create_adjacency_list(preconds,
                                                          statically_consistent_assignments,
                                                          num_parameters,
                                                          adjacency_list,
                                                          num_vertices);
            bron_kerbosch_first_pivot(adjacency_list, vertex_ids, num_parameters, cliques);
            break;
        }

        case BronKerboschMaxNeighborhood: {
            const auto num_vertices = to_vertex_assignment.size();
            vector<uint32_t> adjacency_list[num_vertices];
            const auto vertex_ids = create_adjacency_list(preconds,
                                                          statically_consistent_assignments,
                                                          num_parameters,
                                                          adjacency_list,
                                                          num_vertices);
            bron_kerbosch_max_neighborhood_pivot(
                adjacency_list, vertex_ids, num_parameters, cliques);
            break;
        }

        case BronKerboschMinDifference: {
            const auto num_vertices = to_vertex_assignment.size();
            vector<uint32_t> adjacency_list[num_vertices];
            const auto vertex_ids = create_adjacency_list(preconds,
                                                          statically_consistent_assignments,
                                                          num_parameters,
                                                          adjacency_list,
                                                          num_vertices);
            bron_kerbosch_min_difference_pivot(adjacency_list, vertex_ids, num_parameters, cliques);
            break;
        }

        case KCliqueKPartite: {
            const auto num_vertices = to_vertex_assignment.size();
            std::vector<boost::dynamic_bitset<>> adjacency_matrix(
                num_vertices, boost::dynamic_bitset<>(num_vertices));
            create_adjacency_matrix(preconds,
                                    statically_consistent_assignments,
                                    num_parameters,
                                    num_vertices,
                                    adjacency_matrix);

            const auto &partitions = action_partitions.at(action);
            algorithms::find_all_k_cliques_in_k_partite_graph(
                adjacency_matrix, partitions, cliques);
            break;
        }
        }
    }

    for (const auto &clique : cliques) {
        GroundAtom assignments(num_parameters);
        assignments.resize(num_parameters);

        for (const auto vertex_id : clique) {
            const auto &assignment = to_vertex_assignment.at(vertex_id);
            assignments[assignment.parameter_index] = assignment.object_index;
        }

        const auto action_index = action.get_index();
        const LiftedOperatorId op(action_index, std::move(assignments), unordered_map<int,int>());

        // We do not have to check unary and binary relations, however, if the precondition
        // contains relations of higher arity then we need to check if they hold in the state.

        if (literal_holds(op,
                          preconds.dynamic_preconds,
                          preconds.static_preconds,
                          state,
                          task.get_static_info(),
                          3)) {
            applicable_actions.push_back(op);
        }
    }

    return applicable_actions;
}

std::vector<LiftedOperatorId>
CliqueSuccessorGenerator::get_applicable_actions(const std::vector<ActionSchema> &actions,
                                                 const DBState &state) {
    std::vector<LiftedOperatorId> all_applicable;
    bool has_built_assignment_sets = false;

    for (const auto &action : actions) {
        if (!test_nullary_preconditions(action, state)) {
            continue;
        }

        const auto &action_parameters = action.get_parameters();
        const auto num_parameters = action_parameters.size();

        if (num_parameters == 0) {
            const auto &applicable = applicable_actions_nullary_case(action, state);
            all_applicable.insert(all_applicable.end(), applicable.begin(), applicable.end());
        }
        else if (num_parameters == 1) {
            const auto &applicable = applicable_actions_unary_case(action, state);
            all_applicable.insert(all_applicable.end(), applicable.begin(), applicable.end());
        }
        else {

            if (!has_built_assignment_sets) {
                build_assignment_sets(state);
                has_built_assignment_sets = true;
            }

            const auto &applicable = applicable_actions_general_case(action, state);
            all_applicable.insert(all_applicable.end(), applicable.begin(), applicable.end());
        }
    }

    return all_applicable;
}

vector<LiftedOperatorId>
CliqueSuccessorGenerator::get_applicable_actions(const ActionSchema &action, const DBState &state) {
    if (!test_nullary_preconditions(action, state)) {
        return vector<LiftedOperatorId>();
    }

    const auto &action_parameters = action.get_parameters();
    const auto num_parameters = action_parameters.size();

    if (num_parameters == 0) {
        return applicable_actions_nullary_case(action, state);
    }
    else if (num_parameters == 1) {
        return applicable_actions_unary_case(action, state);
    }
    else {
        build_assignment_sets(state);
        return applicable_actions_general_case(action, state);
    }
}
