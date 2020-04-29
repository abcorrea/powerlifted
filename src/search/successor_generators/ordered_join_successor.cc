#include "ordered_join_successor.h"

#include <vector>
#include <queue>

using namespace std;

vector<Table> OrderedJoinSuccessorGenerator::parse_precond_into_join_program(const vector<Atom> &precond,
                                                                             const DBState &state,
                                                                             const StaticInformation &staticInformation,
                                                                             int action_index) {
    /*
     * See comment in generic join successor.
     */
    priority_queue<Table, vector<Table>, OrderTable> ordered_tables;
    vector<Table> parsed_tables;
    parsed_tables.reserve(precond.size());

    for (const Atom &a : precond) {
        vector<int> indices;
        vector<int> constants;
        get_indices_and_constants_in_preconditions(indices, constants, a);
        vector<GroundAtom> tuples;
        if (!staticInformation.relations[a.predicate_symbol].tuples.empty()) {
            // If this predicate has information in the static information table,
            // then it must be a static predicate
          select_tuples(staticInformation, a, tuples, constants);
            ordered_tables.emplace(move(tuples), move(indices));
        } else {
            // If this predicate does not have information in the static information table,
            // then it must be a fluent
          select_tuples(state, a, tuples, constants);
            ordered_tables.emplace(move(tuples), move(indices));
        }
    }
    while (!ordered_tables.empty()) {
        parsed_tables.push_back(ordered_tables.top());
        ordered_tables.pop();
    }
    return parsed_tables;
}


