#include "random_successor.h"

#include "successor_generator.h"

#include <iostream>
#include <vector>

using namespace std;


vector<Table> RandomSuccessorGenerator::parse_precond_into_join_program(const vector<Atom> &precond,
                                                                       const DBState &state,
                                                                       const StaticInformation &staticInformation,
                                                                       int action_index) {
    /*
     * See the comments in naive_successor.cc
     *
     */
    vector<Table> parsed_tables;
    parsed_tables.reserve(precond.size());
    for (const Atom &a : precond) {
        vector<int> constants;
        vector<int> indices;
        get_indices_and_constants_in_preconditions(indices, constants, a);
        vector<GroundAtom> tuples;
        if (!staticInformation.relations[a.predicate_symbol].tuples.empty()) {
            // If this predicate has information in the static information table,
            // then it must be a static predicate
          select_tuples(staticInformation, a, tuples, constants);
        } else {
            // If this predicate does not have information in the static information table,
            // then it must be a fluent
          select_tuples(state, a, tuples, constants);
        }
        if (!tuples.empty())
            parsed_tables.emplace_back(move(tuples), move(indices));
    }
    shuffle(parsed_tables.begin(), parsed_tables.end(), std::default_random_engine(rand()));
    return parsed_tables;
}