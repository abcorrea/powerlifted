
#include "semi_join.h"
#include "table.h"
#include "utils.h"

#include <algorithm>

using namespace std;

/**
* Semi-join two tables into one. t1 is the working table and will be modified.
*
* @details We first loop over the parameters of each table and check which indices match.
* Then, we split it into two cases:
* 1. If there are no matching indices, then we simply return
* 2. If at least one parameter matches, we perform a nested loop semi-join.
*
* We return the size of the semi-join in order to be able to identify when
* an empty relation is produced.
*
*/
size_t semi_join(Table &t1, const Table &t2) {

    auto matches = compute_matching_columns(t1, t2);

    if (matches.empty()) { // If no attribute matches, then we return
        return t1.tuple_index.size();
    }

    // Otherwise, we perform the join and the projection
    vector<vector<int>> new_tuples;
    for (const vector<int> &tuple_t1 : t1.tuples) {
        for (const vector<int> &tuple_t2 : t2.tuples) {
            bool match = true;
            for (const pair<int, int>& m : matches) {
                if (tuple_t1[m.first] != tuple_t2[m.second]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                // If a tuple matches at least one other tuple, then it is sufficient for the semi-join
                new_tuples.push_back(tuple_t1);
                break;
            }
        }
    }
    t1.tuples = std::move(new_tuples);
    return t1.tuples.size();
}




