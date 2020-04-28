
#include "hash_semi_join.h"
#include "table.h"

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

size_t hash_semi_join(Table &t1, const Table &t2) {
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
                new_tuples.insert(tuple);
            }
        }
    }
    t1.tuples = std::move(new_tuples);
    return t1.tuples.size();
}




