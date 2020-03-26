#include "naive_successor.h"

#include "successor_generator.h"

#include <vector>



using namespace std;

/**
 * @see generic_join_successor.h
 */
vector<Table> NaiveSuccessorGenerator::parse_precond_into_join_program(const vector<Atom> &precond,
                                                                       const State &state,
                                                                       const StaticInformation &staticInformation,
                                                                       int action_index) {

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
            parsed_tables.emplace_back(move(tuples), move(indices));
    }
    return parsed_tables;
}