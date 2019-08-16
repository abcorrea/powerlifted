#include <iostream>
#include <cassert>
#include <vector>
#include <queue>

#include "successor_generator.h"
#include "naive_successor.h"


using namespace std;


vector<Table> NaiveSuccessorGenerator::parse_precond_into_join_program(const vector<Atom> &precond,
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
    return parsed_tables;
}