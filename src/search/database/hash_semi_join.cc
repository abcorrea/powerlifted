#include "hash_semi_join.h"

using namespace std;

void hash_semi_join(Table &t1, Table &t2) {
    /*
     * Semi-join two tables into one.  t1 is the working table and it will be modified
     *
     * We first loop over the parameters of each table and check which indices match.
     * Then, we split it into two cases:
     * 1. If there are no matching indices, then we simply return
     * 2. If at least one parameter matches, we perform a nested loop semi-join.
     *
     */

    vector<pair<int, int>> matches;
    for (int i = 0; i < t1.tuple_index.size(); ++i) {
        for (int j = 0; j < t2.tuple_index.size(); ++j) {
            if (t1.tuple_index[i] == t2.tuple_index[j])
                matches.emplace_back(i, j);
        }
    }

    unordered_set<vector<int>, TupleHash> new_tuples;
    if (matches.empty()) {
        /*
         * If no attribute matches, then we return
         */
        return;
    }
    else {
        /*
         * Otherwise, we perform the join and the projection
         */
        unordered_map<vector<int>, unordered_set<vector<int>, TupleHash>, TupleHash> hash_join_map;
        // Build phase
        for (const vector<int> &tuple : t2.tuples) {
            vector<int> key(matches.size());
            for (int i = 0; i < matches.size(); i++) {
                key[i] = tuple[matches[i].second];
            }
            hash_join_map[key].insert(tuple);
        }

        for (const vector<int> &tuple : t1.tuples) {
            vector<int> key(matches.size());
            for (int i = 0; i < matches.size(); i++) {
                key[i] = tuple[matches[i].first];
            }
            if (hash_join_map.count(key) > 0) {
                new_tuples.insert(tuple);
            }
        }
    }
    t1.tuples = new_tuples;
}




