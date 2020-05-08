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
    : SuccessorGenerator(task), action_data(precompile_action_data(task.actions))
{
}

Table GenericJoinSuccessor::instantiate(const ActionSchema &action,
                                        const DBState &state)
{

    if (action.is_ground()) {
        throw std::runtime_error("Shouldn't be calling instantiate() on a ground action");
    }

    const auto& actiondata = action_data[action.get_index()];

    vector<Table> tables;
    auto res = parse_precond_into_join_program(actiondata, state, tables);

    if (!res) return Table::EMPTY_TABLE();

    assert(!tables.empty());
    assert(tables.size() == actiondata.relevant_precondition_atoms.size());

    Table &working_table = tables[0];
    for (size_t i = 1; i < tables.size(); ++i) {
        hash_join(working_table, tables[i]);
        if (working_table.tuples.size() > largest_intermediate_relation)
            largest_intermediate_relation = working_table.tuples.size();
        // Filter out equalities
        filter_inequalities(action, working_table);
        if (working_table.tuples.empty()) {
            return working_table;
        }
    }

    return working_table;
}

void GenericJoinSuccessor::filter_inequalities(const ActionSchema &action,
                                               Table &working_table) const
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
    for (Argument arg : a.arguments) {
        if (!arg.constant)
            indices.push_back(arg.index);
        else {
            indices.push_back((arg.index + 1) * -1);
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
    for (const GroundAtom &atom : s.get_relations()[a.predicate_symbol].tuples) {
        bool match_constants = true;
        for (int c : constants) {
            assert(a.arguments[c].constant);
            if (atom[c] != a.arguments[c].index) {
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
        bool is_ineq = (p.name == "=");
        if (p.negated and !is_ineq) {
            throw std::runtime_error("Actions with negated preconditions not supported yet");
        }

        // Nullary atoms are handled differently, they don't result in DB tables
        if (!p.arguments.empty() and !is_ineq) {
            data.relevant_precondition_atoms.push_back(p);
        }
    }

    // TODO (GFM): Not sure why this assert is here and why should we fail for it :-)
    assert(!data.relevant_precondition_atoms.empty());

    // Create N empty tables
    data.precompiled_db.resize(data.relevant_precondition_atoms.size());

    for (std::size_t i = 0; i < data.relevant_precondition_atoms.size(); ++i) {
        const Atom &atom = data.relevant_precondition_atoms[i];

        if (!is_static(atom.predicate_symbol)) {
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

        data.precompiled_db[i] = Table(move(tuples), move(indices));
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
        assert(!is_static(atom.predicate_symbol));

        vector<GroundAtom> tuples;
        vector<int> constants, indices;

        // TODO the call next line should be performed at preprocessing as well. We should keep in
        //      adata the vector of constants and indices *for each precondition atom*
        get_indices_and_constants_in_preconditions(indices, constants, atom);
        select_tuples(state, atom, tuples, constants);

        if (tuples.empty()) return false;

        tables[i] = Table(move(tuples), move(indices));
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
        if (p.negated or p.arguments.empty()) {
            continue;
        }
        set<int> args;
        bool has_free_variables = false;
        for (Argument arg : p.arguments) {
            // We parse constants to negative numbers so they're uniquely identified
            if (arg.constant)
                continue;
            has_free_variables = true;
            int node = arg.index;

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