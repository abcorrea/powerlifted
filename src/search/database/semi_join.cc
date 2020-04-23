#include "semi_join.h"

#include <algorithm>

using namespace std;

/**
* Semi-join two tables into one.  t1 is the working table and it will be modified
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
size_t semi_join(Table &t1, Table &t2) {

    vector<pair<int, int>> matches;
    for (size_t i = 0; i < t1.tuple_index.size(); ++i) {
        for (size_t j = 0; j < t2.tuple_index.size(); ++j) {
            if (t1.tuple_index[i] == t2.tuple_index[j])
                matches.emplace_back(i, j);
        }
    }

    unordered_set<vector<int>, TupleHash> new_tuples;
    if (matches.empty()) {
        /*
         * If no attribute matches, then we return
         */
        return t1.tuples.size();
    }
    else {
        /*
         * Otherwise, we perform the join and the projection
         */
        for (const vector<int> &tuple_t1 : t1.tuples) {
            for (const vector<int> &tuple_t2 : t2.tuples) {
                bool match = true;
                for (pair<int, int> m : matches) {
                    if (tuple_t1[m.first] != tuple_t2[m.second]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    // If a tuple matches at least one other tuple,
                    // than it is sufficient for the semi-join
                    new_tuples.insert(tuple_t1);
                    break;
                }
            }
        }
    }
    t1.tuples = new_tuples;
    return t1.tuples.size();
}




