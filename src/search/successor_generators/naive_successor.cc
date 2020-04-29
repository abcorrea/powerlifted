#include "naive_successor.h"

#include "successor_generator.h"

#include <vector>


using namespace std;

/**
 * @see generic_join_successor.h
 */
vector<Table> NaiveSuccessorGenerator::parse_precond_into_join_program(const vector<Atom> &precond,
                                                                       const DBState &state) {

    vector<Table> parsed_tables;
    parsed_tables.reserve(precond.size());
    for (const Atom &a : precond) {
        vector<int> constants;
        vector<int> indices;
        get_indices_and_constants_in_preconditions(indices, constants, a);
        vector<GroundAtom> tuples;
        if (is_static(a.predicate_symbol)) {
            // If this predicate has information in the static information table,
            // then it must be a static predicate
            select_tuples(static_information, a, tuples, constants);
        } else {
            // If this predicate does not have information in the static information table,
            // then it must be a fluent
            select_tuples(state, a, tuples, constants);
        }
        if (!tuples.empty()) {
            parsed_tables.emplace_back(move(tuples), move(indices));
        }
    }
    return parsed_tables;
}