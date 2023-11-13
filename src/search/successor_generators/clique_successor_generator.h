#ifndef SEARCH_CLIQUE_H
#define SEARCH_CLIQUE_H

#include "successor_generator.h"

#include "../action.h"
#include "../action_schema.h"
#include "../states/state.h"
#include "../task.h"

#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

class Task;

struct Assignment {
    int parameter_index;
    int object_index;

    Assignment(int parameter_index, int object_index)
        : parameter_index(parameter_index), object_index(object_index) {
    }
};

struct AssignmentPair {
    std::size_t first_position;
    std::size_t second_position;
    Assignment first_assignment;
    Assignment second_assignment;

    AssignmentPair(std::size_t first_position,
                   const Assignment &first_assignment,
                   std::size_t second_position,
                   const Assignment &second_assignment)
        : first_position(first_position),
          second_position(second_position),
          first_assignment(first_assignment),
          second_assignment(second_assignment) {
    }
};

struct Precondition {
    std::vector<Atom> static_preconds;
    std::vector<Atom> dynamic_preconds;
    std::vector<Atom> dynamic_preconds_with_constants;

    Precondition(const std::vector<Atom> &static_preconds,
                 const std::vector<Atom> &dynamic_preconds)
        : static_preconds(static_preconds),
          dynamic_preconds(dynamic_preconds),
          dynamic_preconds_with_constants() {
        std::sort(this->dynamic_preconds.begin(),
                  this->dynamic_preconds.end(),
                  [](const Atom &lhs, const Atom &rhs) -> bool {
                      size_t lhs_num_free = 0;
                      size_t rhs_num_free = 0;

                      for (const auto &parameter : lhs.get_arguments()) {
                          if (!parameter.is_constant()) {
                              ++lhs_num_free;
                          }
                      }

                      for (const auto &parameter : rhs.get_arguments()) {
                          if (!parameter.is_constant()) {
                              ++rhs_num_free;
                          }
                      }

                      return lhs_num_free > rhs_num_free;
                  });

        for (const auto &atom : this->dynamic_preconds) {
            bool has_constant = false;
            for (const auto &parameter : atom.get_arguments()) {
                if (parameter.is_constant()) {
                    has_constant = true;
                    break;
                }
            }

            if (has_constant) {
                this->dynamic_preconds_with_constants.push_back(atom);
            }
        }
    }
};

enum CliquePivot {
    BronKerboschFirst,
    BronKerboschMaxNeighborhood,
    BronKerboschMinDifference,
    KCliqueKPartite,
};

class CliqueSuccessorGenerator : public SuccessorGenerator {
private:
    const Task &task;
    const CliquePivot pivot;
    std::unordered_map<ActionSchema, std::unordered_map<Parameter, std::set<Object>>> action_objects_by_parameter_type;
    std::unordered_map<ActionSchema, std::vector<Assignment>> action_to_vertex_assignment;
    std::unordered_map<ActionSchema, std::vector<std::vector<std::size_t>>> action_partitions;
    std::unordered_map<ActionSchema, std::vector<AssignmentPair>> action_statically_consistent_assignments;
    std::unordered_map<ActionSchema, Precondition> action_precondition;

    // Cache certain data-structures to avoid dynamic allocations
    std::vector<std::vector<bool>> assignment_sets;

    void build_assignment_sets(const DBState &atoms);

    bool consistent_literals(const std::vector<Atom> &literals,
                             const Assignment &first_assignment,
                             const Assignment &second_assignment);

    bool consistent_literals_with_constants(const std::vector<Atom> &literals);

    bool test_nullary_preconditions(const ActionSchema &action, const DBState &state);

    std::vector<uint32_t> create_adjacency_list(const Precondition &preconds,
                                                const std::vector<AssignmentPair> &statically_consistent_assignments,
                                                std::size_t num_parameters,
                                                std::vector<uint32_t> *adjacency_list,
                                                std::size_t num_vertices);

    void create_adjacency_matrix(const Precondition &preconds,
                                 const std::vector<AssignmentPair> &statically_consistent_assignments,
                                 std::size_t num_parameters,
                                 std::size_t num_vertices,
                                 std::vector<boost::dynamic_bitset<>> &adjacency_matrix);

    std::vector<LiftedOperatorId> applicable_actions_nullary_case(const ActionSchema &action,
                                                                  const DBState &state);

    std::vector<LiftedOperatorId> applicable_actions_unary_case(const ActionSchema &action,
                                                                const DBState &state);

    std::vector<LiftedOperatorId> applicable_actions_general_case(const ActionSchema &action,
                                                                  const DBState &state);

public:
    explicit CliqueSuccessorGenerator(const Task &task, const CliquePivot &pivot);

    std::vector<LiftedOperatorId> get_applicable_actions(const std::vector<ActionSchema> &actions,
                                                         const DBState &state) override;

    std::vector<LiftedOperatorId> get_applicable_actions(const ActionSchema &action,
                                                         const DBState &state) override;

    const GroundAtom tuple_to_atom(const std::vector<int> &tuple, const Atom &eff);

    void apply_nullary_effects(const ActionSchema &action, std::vector<bool> &new_nullary_atoms);

    void apply_ground_action_effects(const ActionSchema &action,
                                     std::vector<Relation> &new_relation);

    void apply_lifted_action_effects(const ActionSchema &action,
                                     const std::vector<int> &tuple,
                                     std::vector<Relation> &new_relation);

    DBState generate_successor(const LiftedOperatorId &op,
                               const ActionSchema &action,
                               const DBState &state) override;
};

#endif  // SEARCH_CLIQUE_H
