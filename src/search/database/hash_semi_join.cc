
#include "hash_semi_join.h"
#include "../hash_structures.h"
#include "table.h"
#include "utils.h"

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

size_t hash_semi_join(Table &t1, const Table &t2) {
    auto matches = compute_matching_columns(t1, t2);

    vector<vector<int>> new_tuples;
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
        unordered_map<vector<int>, unordered_set<vector<int>, TupleHash>, TupleHash> hash_join_map;
        // Build phase
        for (const vector<int> &tuple : t2.tuples) {
            vector<int> key(matches.size());
            for (size_t i = 0; i < matches.size(); i++) {
                key[i] = tuple[matches[i].second];
            }
            hash_join_map[key].insert(tuple);
        }

        for (const vector<int> &tuple : t1.tuples) {
            vector<int> key(matches.size());
            for (size_t i = 0; i < matches.size(); i++) {
                key[i] = tuple[matches[i].first];
            }
            if (hash_join_map.count(key) > 0) {
                new_tuples.push_back(tuple);
            }
        }
    }
    t1.tuples = std::move(new_tuples);
    return t1.tuples.size();
}




