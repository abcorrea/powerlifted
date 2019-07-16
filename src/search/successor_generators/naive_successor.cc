#include <iostream>
#include <cassert>
#include <vector>
#include <queue>

#include "successor_generator.h"
#include "naive_successor.h"


using namespace std;


vector<Table> NaiveSuccessorGenerator::parse_precond_into_join_program(const vector<Atom> &precond, const State &state,
                                                                       const StaticInformation &staticInformation,
                                                                       int action_index) {
    /*
     * We first parse the state and the atom preconditions into a set of tables
     * to perform the join-program more easily.
     */
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
            parsed_tables.emplace_back(staticInformation.relations[a.predicate_symbol].tuples, indices);
        } else {
            // If this predicate does not have information in the static information table,
            // then it must be a fluent
            parsed_tables.emplace_back(state.relations[a.predicate_symbol].tuples, indices);
        }
    }
    return parsed_tables;
}
