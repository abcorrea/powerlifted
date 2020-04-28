#include "generic_join_successor.h"

#include "../database/hash_join.h"
#include "../database/semi_join.h"

#include <cassert>
#include <vector>

using namespace std;

Table GenericJoinSuccessor::instantiate(const ActionSchema &action,
                                        const DBState &state,
                                        const StaticInformation &staticInformation)
{
    /*
     * We first certify that there are preconditions for the action.
     *
     * IMPORTANT:
     *    - We only perform the join over the POSITIVE preconditions, NEGATIVE preconditions
     *      are not supported by now
     */
    std::vector<std::vector<int>> instantiations;
    const std::vector<Parameter> &params = action.get_parameters();

    if (params.empty()) {
        return Table();
    }
    std::vector<Atom> precond;
    for (const Atom &p : action.get_precondition()) {
        // Ignoring negative preconditions when instantiating
        if ((!p.negated) and p.arguments.size() > 0) {
            precond.push_back((p));
        }
    }
    assert(!precond.empty());

    vector<Table> tables =
        parse_precond_into_join_program(precond, state, staticInformation, action.get_index());
    assert(!tables.empty());
    if (tables.size() != precond.size()) {
        // This means that the projection over the constants completely eliminated one table,
        // we can return no instantiation.
        return Table();
    }
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
    /*
     * Loop over inequalities and remove those not consistent with the
     * current instantiation
     */
    for (const pair<int, int> ineq : action.get_inequalities()) {
        auto it_1 =
            find(working_table.tuple_index.begin(), working_table.tuple_index.end(), ineq.first);
        auto it_2 =
            find(working_table.tuple_index.begin(), working_table.tuple_index.end(), ineq.second);
        int index1 = distance(working_table.tuple_index.begin(), it_1);
        int index2 = distance(working_table.tuple_index.begin(), it_2);
        if (it_1 != working_table.tuple_index.end() and it_2 != working_table.tuple_index.end()) {
            vector<vector<int>> to_remove;
            for (auto &&t : working_table.tuples) {
                if (t[index1] == t[index2])
                    to_remove.push_back(t);
            }
            for (auto &&t : to_remove) {
                working_table.tuples.erase(t);
            }
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
                                         unordered_set<GroundAtom, TupleHash> &tuples,
                                         const std::vector<int> &constants)
{
    bool match_constants;
    for (const GroundAtom &atom : s.relations[a.predicate_symbol].tuples) {
        match_constants = true;
        for (int c : constants) {
            assert(a.arguments[c].constant);
            if (atom[c] != a.arguments[c].index)
                match_constants = false;
        }
        if (match_constants)
            tuples.insert(atom);
    }
}

vector<Table>
GenericJoinSuccessor::parse_precond_into_join_program(const vector<Atom> &precond,
                                                      const DBState &state,
                                                      const StaticInformation &staticInformation,
                                                      int action_index)
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
    vector<Table> parsed_tables;
    parsed_tables.reserve(precond.size());
    for (const Atom &a : precond) {
        vector<int> constants;
        vector<int> indices;
        get_indices_and_constants_in_preconditions(indices, constants, a);
        unordered_set<GroundAtom, TupleHash> tuples;
        if (!staticInformation.relations[a.predicate_symbol].tuples.empty()) {
            // If this predicate has information in the static information table,
            // then it must be a static predicate
            select_tuples(staticInformation, a, tuples, constants);
        }
        else {
            // If this predicate does not have information in the static information table,
            // then it must be a fluent
            select_tuples(state, a, tuples, constants);
        }
        if (!tuples.empty())
            parsed_tables.emplace_back(move(tuples), move(indices));
    }
    return parsed_tables;
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
            int node;
            if (arg.constant)
                continue;
            has_free_variables = true;
            node = arg.index;

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