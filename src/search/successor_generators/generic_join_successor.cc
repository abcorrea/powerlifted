#include "../action.h"
#include "successor_generator.h"
#include "generic_join_successor.h"

#include "../action_schema.h"
#include "../database/hash_join.h"
#include "../database/semi_join.h"
#include "../database/table.h"
#include "../states/state.h"
#include "../task.h"

#include <algorithm>
#include <cassert>
#include <vector>

using namespace std;

GenericJoinSuccessor::GenericJoinSuccessor(const Task &task)
    : static_information(task.get_static_info()), is_predicate_static(), action_data()
{
    is_predicate_static.reserve(static_information.get_relations().size());
    for (const auto &r : static_information.get_relations()) {
        is_predicate_static.push_back(!r.tuples.empty());
    }
    action_data = precompile_action_data(task.get_action_schemas());
}

Table GenericJoinSuccessor::instantiate(const ActionSchema &action,
                                        const DBState &state)
{

    if (action.is_ground()) {
        throw std::runtime_error("Shouldn't be calling instantiate() on a ground action");
    }

    const auto& actiondata = action_data[action.get_index()];

    vector<Table> tables(0);
    auto res = parse_precond_into_join_program(actiondata, state, tables);

    if (!res) return Table::EMPTY_TABLE();

    assert(!tables.empty());
    assert(tables.size() == actiondata.relevant_precondition_atoms.size());

    Table &working_table = tables[0];
    for (size_t i = 1; i < tables.size(); ++i) {
        hash_join(working_table, tables[i]);
        // Filter out equalities
        filter_inequalities(action, working_table);
        if (working_table.tuples.empty()) {
            return working_table;
        }
    }

    return working_table;
}

void GenericJoinSuccessor::filter_inequalities(const ActionSchema &action,
                                               Table &working_table)
{
    const auto& tup_idx = working_table.tuple_index;

    // Loop over inequalities and remove those not consistent with the current instantiation
    for (const pair<int, int>& ineq : action.get_inequalities()) {
        // TODO Revise this, looks that some work could be offloaded to preprocessing so that we
        //      do not need to do all this linear-time finds at runtime?
        auto it_1 = find(tup_idx.begin(), tup_idx.end(), ineq.first);
        auto it_2 = find(tup_idx.begin(), tup_idx.end(), ineq.second);

        if (it_1 != tup_idx.end() and it_2 != tup_idx.end()) {
            int index1 = distance(tup_idx.begin(), it_1);
            int index2 = distance(tup_idx.begin(), it_2);

            vector<vector<int>> newtuples;
            for (const auto &t : working_table.tuples) {
                if (t[index1] != t[index2]) {
                    newtuples.push_back(t);
                }
            }
            working_table.tuples = std::move(newtuples);
        }
    }
}

void GenericJoinSuccessor::get_indices_and_constants_in_preconditions(vector<int> &indices,
                                                                      vector<int> &constants,
                                                                      const Atom &a)
{
    int cont = 0;
    for (Argument arg : a.get_arguments()) {
        if (!arg.is_constant())
            indices.push_back(arg.get_index());
        else {
            indices.push_back((arg.get_index() + 1) * -1);
            constants.push_back(cont);
        }
        cont++;
    }
}

/*
 * Select only those tuples matching the constants of a partially grounded
 * precondition.
 */
void GenericJoinSuccessor::select_tuples(const DBState &s,
                                         const Atom &a,
                                         std::vector<GroundAtom> &tuples,
                                         const std::vector<int> &constants)
{
    for (const GroundAtom &atom : s.get_relations()[a.get_predicate_symbol_idx()].tuples) {
        bool match_constants = true;
        for (int c : constants) {
            assert(a.get_arguments()[c].is_constant());
            if (atom[c] != a.get_arguments()[c].get_index()) {
                match_constants = false;
                break;
            }
        }
        if (match_constants) tuples.push_back(atom);
    }
}

std::vector<PrecompiledActionData>
GenericJoinSuccessor::precompile_action_data(const std::vector<ActionSchema>& actions) {
    std::vector<PrecompiledActionData> result;
    result.reserve(actions.size());
    for (const auto &a:actions) {
        result.push_back(precompile_action_data(a));
    }
    return result;
}

PrecompiledActionData GenericJoinSuccessor::precompile_action_data(const ActionSchema& action) {
    PrecompiledActionData data;

    data.is_ground = action.get_parameters().empty();
    if (data.is_ground) return data; // We won't need anything from this action


    for (const Atom &p : action.get_precondition()) {
        bool is_ineq = (p.get_name() == "=");
        if (p.is_negated() and !is_ineq) {
            throw std::runtime_error("Actions with negated preconditions not supported yet");
        }

        // Nullary atoms are handled differently, they don't result in DB tables
        if (!p.is_ground() and !is_ineq) {
            data.relevant_precondition_atoms.push_back(p);
        }
    }

    // TODO (GFM): Not sure why this assert is here and why should we fail for it :-)
    assert(!data.relevant_precondition_atoms.empty());

    // Create N empty tables
    data.precompiled_db.resize(data.relevant_precondition_atoms.size());

    for (std::size_t i = 0; i < data.relevant_precondition_atoms.size(); ++i) {
        const Atom &atom = data.relevant_precondition_atoms[i];

        if (!is_static(atom.get_predicate_symbol_idx())) {
            // If the atom is fluent, we just flag it as such and we're done: we'll have to deal
            // with it during search time
            data.fluent_tables.push_back(i);
            continue;
        }

        // Otherwise the atom is static, so we precompile the table corresponding to it
        vector<GroundAtom> tuples;
        vector<int> constants, indices;

        get_indices_and_constants_in_preconditions(indices, constants, atom);

        select_tuples(static_information, atom, tuples, constants);

        if (tuples.empty()) {
            data.statically_inapplicable = true;
            return data;
        }

        data.precompiled_db[i] = Table(std::move(tuples), std::move(indices));
    }

    return data;
}



bool GenericJoinSuccessor::parse_precond_into_join_program(
    const PrecompiledActionData &adata, const DBState &state, std::vector<Table>& tables)
{
    /*
     * Parse the state and the atom preconditions into a set of tables
     * to perform the join-program more easily.
     *
     * We first obtain all indices in the precondition that are constants.
     * Then, we create the table applying the projection over the arguments
     * that satisfy the instantiation of the constants. There are two cases
     * for the projection:
     *    1. The table comes from the static information; or
     *    2. The table comes directly from the current state.
     *
     */
    if (adata.statically_inapplicable) return false;

    tables = adata.precompiled_db;  // This performs the copy that we'll return
    for (unsigned i:adata.fluent_tables) {
        // Let's fill in those (currently empty) tables that correspond to
        // fluent symbols in the precondition
        const Atom &atom = adata.relevant_precondition_atoms[i];
        assert(!is_static(atom.get_predicate_symbol_idx()));

        vector<GroundAtom> tuples;
        vector<int> constants, indices;

        // TODO the call next line should be performed at preprocessing as well. We should keep in
        //      adata the vector of constants and indices *for each precondition atom*
        get_indices_and_constants_in_preconditions(indices, constants, atom);
        select_tuples(state, atom, tuples, constants);

        if (tuples.empty()) return false;

        tables[i] = Table(std::move(tuples), std::move(indices));
    }

    return true;
}


/*
 * Create hypergraph of precondition
 *
 * Loop through every precondition and filters out negated and nullary atoms.
 * Then, assign each free variable of the precondition to a corresponding index
 * and create the hyperedge of the vertice with these indices.
 *
 * If there is no free variable in a precondition (i.e., ground atom
 * precondition), we add this precondition to a list so we know we need
 * to join it after performing the full-reducer/Yannakakis.
 *
 */
void GenericJoinSuccessor::create_hypergraph(const ActionSchema &action,
                                             vector<int> &hypernodes,
                                             vector<set<int>> &hyperedges,
                                             vector<int> &missing_precond,
                                             map<int, int> &node_index,
                                             map<int, int> &node_counter,
                                             map<int, int> &edge_to_precond)
{
    int cont = 0;
    for (const Atom &p : action.get_precondition()) {
        bool is_ineq = (p.get_name() == "=");
        if (p.is_negated() or p.is_ground() or is_ineq) {
            continue;
        }
        set<int> args;
        bool has_free_variables = false;
        for (Argument arg : p.get_arguments()) {
            // We parse constants to negative numbers so they're uniquely identified
            if (arg.is_constant())
                continue;
            has_free_variables = true;
            int node = arg.get_index();

            args.insert(node);
            if (find(hypernodes.begin(), hypernodes.end(), node) == hypernodes.end()) {
                node_index[node] = hypernodes.size();
                node_counter[node] = 1;
                hypernodes.push_back(node);
            }
            else {
                node_counter[node] = node_counter[node] + 1;
            }
        }
        if (!args.empty() and has_free_variables) {
            // map ith-precondition to a given edge
            edge_to_precond[hyperedges.size()] = cont;
            hyperedges.emplace_back(args.begin(), args.end());
        }
        else {
            // If all args of a preconditions are constant, we check it first
            missing_precond.push_back(cont);
        }
        ++cont;
    }
}

DBState GenericJoinSuccessor::generate_successor(
    const LiftedOperatorId &op,
    const ActionSchema& action,
    const DBState &state) {

    added_atoms.clear();
    vector<bool> new_nullary_atoms(state.get_nullary_atoms());
    vector<Relation> new_relation(state.get_relations());
    apply_nullary_effects(action, new_nullary_atoms);

    if (action.is_ground()) {
        apply_ground_action_effects(action, new_relation);
    }
    else {
        apply_lifted_action_effects(action, op.get_instantiation(), new_relation);
    }

    return DBState(std::move(new_relation), std::move(new_nullary_atoms));
}

void GenericJoinSuccessor::order_tuple_by_free_variable_order(const vector<int> &free_var_indices,
                                                            const vector<int> &map_indices_to_position,
                                                            const vector<int> &tuple_with_const,
                                                            vector<int> &ordered_tuple) {
    for (size_t i = 0; i < free_var_indices.size(); ++i) {
        ordered_tuple[free_var_indices[i]] = tuple_with_const[map_indices_to_position[i]];
    }
}
void GenericJoinSuccessor::compute_map_indices_to_table_positions(const Table &instantiations,
                                                                vector<int> &free_var_indices,
                                                                vector<int> &map_indices_to_position) {
    for (size_t j = 0; j < instantiations.tuple_index.size(); ++j) {
        if (instantiations.index_is_variable(j)) {
            free_var_indices.push_back(instantiations.tuple_index[j]);
            map_indices_to_position.push_back(j);
        }
    }
}
bool GenericJoinSuccessor::is_trivially_inapplicable(const DBState &state, const ActionSchema &action) {
    const auto& positive_precond = action.get_positive_nullary_precond();
    const auto& negative_precond = action.get_negative_nullary_precond();
    const auto& nullary_atoms = state.get_nullary_atoms();
    for (size_t i = 0; i < positive_precond.size(); ++i) {
        if ((positive_precond[i] and !nullary_atoms[i]) or
            (negative_precond[i] and nullary_atoms[i])) {
            return true;
        }
    }
    return false;
}
void GenericJoinSuccessor::apply_nullary_effects(const ActionSchema &action,
                                               vector<bool> &new_nullary_atoms)
{
    /*
     * Loop over positive and negative nullary effects and apply them accordingly
     * to the state.
     */
    for (size_t i = 0; i < action.get_negative_nullary_effects().size(); ++i) {
        if (action.get_negative_nullary_effects()[i])
            new_nullary_atoms[i] = false;
    }
    for (size_t i = 0; i < action.get_positive_nullary_effects().size(); ++i) {
        if (action.get_positive_nullary_effects()[i]) {
            new_nullary_atoms[i] = true;
            add_to_added_atoms(i, GroundAtom());
        }
    }
}
void GenericJoinSuccessor::apply_ground_action_effects(const ActionSchema &action,
                                                     vector<Relation> &new_relation)
{
    for (const Atom &eff : action.get_effects()) {
        GroundAtom ga;
        for (const Argument &a : eff.get_arguments()) {
            // Create ground atom for each effect given the instantiation
            assert(a.is_constant());
            ga.push_back(a.get_index());
        }
        assert(eff.get_predicate_symbol_idx() == new_relation[eff.get_predicate_symbol_idx()].predicate_symbol);
        if (eff.is_negated()) {
            // If ground effect is negated, remove it from relation
            new_relation[eff.get_predicate_symbol_idx()].tuples.erase(ga);
        }
        else {
            // If ground effect is not in the state, we add it

            new_relation[eff.get_predicate_symbol_idx()].tuples.insert(ga);
            add_to_added_atoms(eff.get_predicate_symbol_idx(), ga);

        }
    }
}
void GenericJoinSuccessor::apply_lifted_action_effects(const ActionSchema &action,
                                                     const vector<int> &tuple,
                                                     vector<Relation> &new_relation)
{
    for (const Atom &eff : action.get_effects()) {
        GroundAtom ga = GenericJoinSuccessor::tuple_to_atom(tuple, eff);
        assert(eff.get_predicate_symbol_idx() == new_relation[eff.get_predicate_symbol_idx()].predicate_symbol);
        if (eff.is_negated()) {
            // Remove from relation
            new_relation[eff.get_predicate_symbol_idx()].tuples.erase(ga);
        }
        else {
            int predicate_symbol_idx = eff.get_predicate_symbol_idx();
            if (find(new_relation[predicate_symbol_idx].tuples.begin(),
                     new_relation[predicate_symbol_idx].tuples.end(),
                     ga) == new_relation[predicate_symbol_idx].tuples.end()) {
                // If ground atom is not in the state, we add it

                new_relation[eff.get_predicate_symbol_idx()].tuples.insert(ga);
                add_to_added_atoms(eff.get_predicate_symbol_idx(), ga);

            }
        }
    }
}

/**
 * @implementation We first check if the nullary preconditions
 * are satisfied in the current state. Then we check if
 * there is any instantiation of the action schema in the given state. If there
 * is none, then two cases are possible:
 *    1. The action schema is not applicable. In this case, we just proceed to
 *    instantiate the next schema; or
 *    2. The action schema is ground. In this case, we simply proceed to check
 *    if the preconditions are satisfied and, if so, apply the ground action. We
 *    need to check applicability here because, if there is no parameter, then
 *    the join in the successor generator was never performed.
 * If there are instantiations, then we simply apply the action effects, since
 * we know the actions are applicable.
 */
std::vector<LiftedOperatorId> GenericJoinSuccessor::get_applicable_actions(
        const ActionSchema &action, const DBState &state)
{
    std::vector<LiftedOperatorId> applicable;
    if (is_trivially_inapplicable(state, action)) {
        return applicable;
    }

    if (action.is_ground()) {
        if (is_ground_action_applicable(action, state)) {
            applicable.emplace_back(action.get_index(), vector<int>());
        }
        return applicable;
    }

    Table instantiations = instantiate(action, state);
    if (instantiations.tuples.empty()) { // No applicable action from this schema
        return applicable;
    }

    vector<int> free_var_indices;
    vector<int> map_indices_to_position;
    compute_map_indices_to_table_positions(
        instantiations, free_var_indices, map_indices_to_position);

    for (const vector<int> &tuple_with_const : instantiations.tuples) {
        vector<int> ordered_tuple(free_var_indices.size());
        order_tuple_by_free_variable_order(
            free_var_indices, map_indices_to_position, tuple_with_const, ordered_tuple);
        applicable.emplace_back(action.get_index(), std::move(ordered_tuple));
    }
    return applicable;
}


/**
 *    This action generates the ground atom produced by an atomic effect given an instantiation of
 *    its parameters.
 *
 *    @details First, we rearrange the indices. Then, we create the atom based on whether the
 * argument is a constant or not. If it is, then we simply pass the constant value; otherwise we use
 *    the instantiation that we found.
 */
const GroundAtom GenericJoinSuccessor::tuple_to_atom(const vector<int> &tuple, const Atom &eff)
{
    GroundAtom ground_atom;
    ground_atom.reserve(eff.get_arguments().size());
    for (auto argument : eff.get_arguments()) {
        if (!argument.is_constant())
            ground_atom.push_back(tuple[argument.get_index()]);
        else
            ground_atom.push_back(argument.get_index());
    }

    // Sanity check: check that all positions of the tuple were initialized
    assert(find(ground_atom.begin(), ground_atom.end(), -1) == ground_atom.end());

    return ground_atom;
}
/*
 * Check the applicability of an already ground action (given grounded in the
 * PDDL). We just need to check applicability for completely ground actions
 * because the successor generations find only applicable actions for lifted
 * ones.
 *
 * In this case, the parameter type is slightly misleading, but the parameter
 * 'action' is a ground action here.
 */
bool GenericJoinSuccessor::is_ground_action_applicable(const ActionSchema &action,
                                                       const DBState &state) const
{
    for (const Atom &precond : action.get_precondition()) {
        int index = precond.get_predicate_symbol_idx();
        vector<int> tuple;
        tuple.reserve(precond.get_arguments().size());
        for (const Argument &arg : precond.get_arguments()) {
            assert(arg.is_constant());
            tuple.push_back(arg.get_index());  // Index of a constant is the obj index
        }
        const auto& tuples_in_relation = state.get_tuples_of_relation(index);
        const auto& it_end_tuples_in_relation = tuples_in_relation.end();
        const auto& static_tuples = get_tuples_from_static_relation(index);
        const auto& it_end_static_tuples = static_tuples.end();
        if (!tuples_in_relation.empty()) {
            if (precond.is_negated()) {
                if (tuples_in_relation.find(tuple) != it_end_tuples_in_relation)
                    return false;
            }
            else {
                if (tuples_in_relation.find(tuple) == it_end_tuples_in_relation)
                    return false;
            }
        }
        else if (!static_tuples.empty()) {
            if (precond.is_negated()) {
                if (static_tuples.find(tuple) != it_end_static_tuples)
                    return false;
            }
            else {
                if (static_tuples.find(tuple) == it_end_static_tuples)
                    return false;
            }
        }
        else {
            return false;
        }
    }
    return true;
}
const unordered_set<GroundAtom, TupleHash> &
GenericJoinSuccessor::get_tuples_from_static_relation(size_t i) const
{
    return static_information.get_tuples_of_relation(i);
}
