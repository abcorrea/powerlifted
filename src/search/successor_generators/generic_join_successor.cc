#include <iostream>
#include <cassert>
#include <vector>
#include <queue>


#include "generic_join_successor.h"

#include "../database/hash_join.h"

Table GenericJoinSuccessor::instantiate(const ActionSchema &action, const State &state,
                                           const StaticInformation &staticInformation) {
    /*
     * We first certify that there are preconditions for the action.
     *
     * IMPORTANT:
     *    - We only perform the join over the POSITIVE preconditions, NEGATIVE preconditions
     *      are not supported by now
     */
    vector<vector<int>> instantiations;
    const vector<Parameter> &params = action.getParameters();

    if (params.empty()) {
        return Table();
    }


    vector<Atom> precond;
    for (const Atom &p : action.getPrecondition()) {
        // Ignoring negative preconditions when instantiating
        if ((!p.negated) and p.tuples.size() > 0) {
            precond.push_back((p));
        }
    }

    assert (!precond.empty());

    vector<Table> tables = parse_precond_into_join_program(precond, state, staticInformation, action.getIndex());
    assert (!tables.empty());
    if (tables.size() != precond.size()) {
        // This means that the projection over the constants completely eliminated one table,
        // we can return no instantiation.
        return Table();
    }
    Table &working_table = tables[0];
    for (int i = 1; i < tables.size(); ++i) {
        hash_join(working_table, tables[i]);
        if (working_table.tuples.size() > largest_intermediate_relation)
            largest_intermediate_relation = working_table.tuples.size();
        // Filter out equalities
        for (const pair<int, int> ineq : action.getInequalities()) {
            auto it_1 = find(working_table.tuple_index.begin(),
                             working_table.tuple_index.end(),
                             ineq.first);
            auto it_2 = find(working_table.tuple_index.begin(),
                             working_table.tuple_index.end(),
                             ineq.second);
            int index1 = distance(working_table.tuple_index.begin(), it_1);
            int index2 = distance(working_table.tuple_index.begin(), it_2);
            if (it_1 != working_table.tuple_index.end() and it_2 != working_table.tuple_index.end()) {
                vector<vector<int>> to_remove;
                for (auto && t : working_table.tuples) {
                    if (t[index1] == t[index2])
                        to_remove.push_back(t);
                }
                for (auto &&t : to_remove) {
                    working_table.tuples.erase(t);
                }
            }
        }
        if (working_table.tuples.empty()) {
            return working_table;
        }
    }

    return working_table;
}


const void GenericJoinSuccessor::get_indices_and_constants_in_preconditions(vector<int> &indices,
                                                                          vector<int> &constants,
                                                                          const Atom &a) {
    int cont = 0;
    for (Argument arg : a.tuples) {
        if (!arg.constant)
            indices.push_back(arg.index);
        else {
            indices.push_back((arg.index+1) * -1);
            constants.push_back(cont);
        }
        cont++;
    }
}

const void GenericJoinSuccessor::project_tuples(const State &s,
                                              const Atom &a,
                                              unordered_set<GroundAtom, TupleHash> &tuples,
                                              const std::vector<int> &constants) {
    bool match_constants;
    for (const GroundAtom &atom : s.relations[a.predicate_symbol].tuples) {
        match_constants = true;
        for (int c : constants) {
            assert (a.tuples[c].constant);
            if (atom[c] != a.tuples[c].index)
                match_constants = false;
        }
        if (match_constants)
            tuples.insert(atom);
    }
}



vector<Table> GenericJoinSuccessor::parse_precond_into_join_program(const vector<Atom> &precond,
                                                                             const State &state,
                                                                             const StaticInformation &staticInformation,
                                                                             int action_index) {
    /*
     * Parse the state and the atom preconditions into a set of tables
     * to perform the join-program more easily.
     *
     * We first obtain all indices in the precondition that are constants.
     * Then, we create the table applying the projection over the tuples
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
            project_tuples(staticInformation, a, tuples, constants);
        } else {
            // If this predicate does not have information in the static information table,
            // then it must be a fluent
            project_tuples(state, a, tuples, constants);
        }
        if (!tuples.empty())
            parsed_tables.emplace_back(tuples, indices);
    }
    shuffle(parsed_tables.begin(), parsed_tables.end(), std::default_random_engine(rand()));
    return parsed_tables;
}