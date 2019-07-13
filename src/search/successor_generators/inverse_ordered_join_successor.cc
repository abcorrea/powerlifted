#include <iostream>
#include <cassert>
#include <vector>
#include <queue>

#include "inverse_ordered_join_successor.h"

using namespace std;

vector<Table> InverseOrderedJoinSuccessorGenerator::parse_precond_into_join_program(const vector<Atom> &precond, const State &state,
                                                                             const StaticInformation &staticInformation) {
    /*
     * We first parse the state and the atom preconditions into a set of tables
     * to perform the join-program more easily.
     */
    priority_queue<Table, vector<Table>, InverseOrderTable> ordered_tables;
    vector<Table> parsed_tables;//(precond.size());
    parsed_tables.reserve(precond.size());
    for (const Atom &a : precond) {
        vector<int> indices;
        for (Argument arg : a.tuples) {
            indices.push_back(arg.index);
        }
        if (!staticInformation.relations[a.predicate_symbol].tuples.empty()) {
            // If this predicate has information in the static information table,
            // then it must be a static predicate
            ordered_tables.emplace(staticInformation.relations[a.predicate_symbol].tuples, indices);
        } else {
            // If this predicate does not have information in the static information table,
            // then it must be a fluent
            ordered_tables.emplace(state.relations[a.predicate_symbol].tuples, indices);
        }
    }
    while (!ordered_tables.empty()) {
        parsed_tables.push_back(ordered_tables.top());
        ordered_tables.pop();
    }
    return parsed_tables;
}


