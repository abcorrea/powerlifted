#include "../action.h"
#include "successor_generator.h"
#include "generic_join_successor.h"

#include "../action_schema.h"
#include "../axiom_evaluator.h"
#include "../database/hash_join.h"
#include "../database/table.h"
#include "../states/state.h"
#include "../task.h"

#include <algorithm>
#include <cassert>
#include <vector>

using namespace std;

GenericJoinSuccessor::GenericJoinSuccessor(const Task &task)
    : static_information(task.get_static_info()),
      axiom_evaluator(task.get_axiom_evaluator()),
      is_predicate_static(), action_data()
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
    assert(tables.size() == actiondata.relevant_atoms.size());

    Table &working_table = tables[0];
    std::vector<bool> applied(action.get_static_precondition().size(), false);
    for (size_t i = 1; i < tables.size(); ++i) {
        hash_join(working_table, tables[i]);
        // Filter out equalities
        filter_static(action, working_table, applied);
        if (working_table.tuples.empty()) {
            return working_table;
        }
    }

    return working_table;
}

void GenericJoinSuccessor::filter_static(const ActionSchema &action,
                                         Table &working_table,
                                         std::vector<bool> &applied)
{
    // TODO: for now, we assume all static preconditions are
    //       (in)equalities. This may change in future
    join_program::filter_equalities(action.get_static_precondition(),
                                    working_table, applied);
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
    if (action.get_parameters().empty()) {
        // We won't need anything from this action
        PrecompiledActionData data;
        data.is_ground = true;
        return data;
    }

    vector<Atom> relevant_atoms;
    for (const Atom &p : action.get_precondition()) {
        bool is_ineq = (p.get_name() == "=");
        if (p.is_negated() and !is_ineq) {
            throw std::runtime_error("Actions with negated preconditions not supported yet");
        }

        // Nullary atoms are handled differently, they don't result in DB tables
        if (!p.is_ground() and !is_ineq) {
            relevant_atoms.push_back(p);
        }
    }

    // TODO (GFM): Not sure why this assert is here and why should we fail for it :-)
    assert(!relevant_atoms.empty());

    return join_program::precompile(std::move(relevant_atoms),
                                    is_predicate_static, static_information);
}



bool GenericJoinSuccessor::parse_precond_into_join_program(
    const PrecompiledActionData &adata, const DBState &state, std::vector<Table>& tables)
{
    return join_program::fill_tables(adata, state, tables);
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
        apply_ground_action_effects(action, new_relation, op.get_fresh_vars_mapping());
    }
    else {
        apply_lifted_action_effects(action, op.get_instantiation(),
                                    new_relation, op.get_fresh_vars_mapping());
    }

    DBState successor(std::move(new_relation), std::move(new_nullary_atoms),
                      state.get_number_objects()+action.get_fresh_variables().size());
    // Recompute the derived predicates: the relations copied from the parent
    // contain the parent's derived atoms, which the evaluator discards and
    // rederives from the new fluents. Every state that leaves the successor
    // generator is therefore fully evaluated, which the packed (duplicate
    // detection) representation relies on.
    axiom_evaluator.evaluate(successor);
    return successor;
}

void GenericJoinSuccessor::order_tuple_by_free_variable_order(const vector<int> &free_var_indices,
                                                            const vector<int> &map_indices_to_position,
                                                            const Table::tuple_t &tuple_with_const,
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
                                                       vector<Relation> &new_relation,
                                                       std::unordered_map<int, int> new_objs)
{
    for (const Atom &eff : action.get_effects()) {
        GroundAtom ga;
        for (const Argument &a : eff.get_arguments()) {
            // Create ground atom for each effect given the instantiation
            if (a.is_constant())
                ga.push_back(a.get_index());
            else
                ga.push_back(new_objs.at(a.get_index()));
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
                                                       vector<Relation> &new_relation,
                                                       unordered_map<int, int> new_objs)
{
    for (const Atom &eff : action.get_effects()) {
        GroundAtom ga = GenericJoinSuccessor::tuple_to_atom(tuple, eff, new_objs);
        assert(eff.get_predicate_symbol_idx() == new_relation[eff.get_predicate_symbol_idx()].predicate_symbol);
        if (eff.is_negated()) {
            // Remove from relation
            new_relation[eff.get_predicate_symbol_idx()].tuples.erase(ga);
        }
        else {
            int predicate_symbol_idx = eff.get_predicate_symbol_idx();
            // tuples is an unordered_set, so insert() both adds the atom and
            // tells us (via .second) whether it was new — no need for a prior
            // O(n) linear std::find over the set.
            auto insertion = new_relation[predicate_symbol_idx].tuples.insert(ga);
            if (insertion.second) {
                // Ground atom was not already in the state: record it as added.
                add_to_added_atoms(predicate_symbol_idx, ga);
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
    std::unordered_map<int, int> new_objs;
    int new_obj_idx = state.get_number_objects();
    for (const FreshVariable &arg : action.get_fresh_variables()) {
        new_objs[arg.get_index()] = new_obj_idx++;
    }

    std::vector<LiftedOperatorId> applicable;
    if (is_trivially_inapplicable(state, action)) {
        return applicable;
    }

    if (action.is_ground()) {
        if (is_ground_action_applicable(action, state)) {
            applicable.emplace_back(action.get_index(), vector<int>(), new_objs);
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

    for (const auto &tuple_with_const : instantiations.tuples) {
        vector<int> ordered_tuple(free_var_indices.size());
        order_tuple_by_free_variable_order(
            free_var_indices, map_indices_to_position, tuple_with_const, ordered_tuple);
        applicable.emplace_back(action.get_index(), std::move(ordered_tuple), new_objs);
    }
    return applicable;
}

std::vector<LiftedOperatorId> GenericJoinSuccessor::get_applicable_actions(
            const std::vector<ActionSchema> &actions, const DBState &state)
{
    std::vector<LiftedOperatorId> all_applicable_actions;

    for (const auto& action : actions) {
        const auto applicable_actions = get_applicable_actions(action, state);
        all_applicable_actions.reserve(all_applicable_actions.size() + applicable_actions.size());
        all_applicable_actions.insert(all_applicable_actions.end(), applicable_actions.cbegin(), applicable_actions.cend());
    }

    return all_applicable_actions;
}

/**
 *    This action generates the ground atom produced by an atomic effect given an instantiation of
 *    its parameters.
 *
 *    @details First, we rearrange the indices. Then, we create the atom based on whether the
 * argument is a constant or not. If it is, then we simply pass the constant value; otherwise we use
 *    the instantiation that we found.
 */
const GroundAtom GenericJoinSuccessor::tuple_to_atom(const vector<int> &tuple,
                                                     const Atom &eff,
                                                     const unordered_map<int, int> &new_objs)
{
    GroundAtom ground_atom;
    ground_atom.reserve(eff.get_arguments().size());
    for (auto argument : eff.get_arguments()) {
        if (!argument.is_constant())
            if (!argument.is_fresh_var())
                ground_atom.push_back(tuple[argument.get_index()]);
            else {
                assert(new_objs.count(argument.get_index()) > 0);
                ground_atom.push_back(new_objs.at(argument.get_index()));
            }
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
        GroundAtom tuple;
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
