
#include "project.h"
#include "table.h"

#include <unordered_map>
#include <vector>

using namespace std;

void project(Table &t, const std::unordered_set<int> &over) {

    /*
     * This projection is not a canonical projection. We still need to keep *a full assignment*
     * for every column of the table so we can instantiate the action later on. Thus, we DO NOT
     * remove the indices from the table here.
     */

    vector<int> matches;
    for (int x : over)
        for (size_t i = 0; i < t.tuple_index.size(); i++)
            if (x == t.tuple_index[i])
                matches.push_back(i);

    unordered_map<vector<int>, vector<int>, TupleHash> hash_map;

    for (const vector<int> &tuple : t.tuples) {
        vector<int> key(matches.size());
        for (size_t i = 0; i < matches.size(); i++) {
                key[i] = tuple[matches[i]];
            }
        if (hash_map.count(key) == 0)
            hash_map[key] = tuple;
    }

    vector<vector<int>> new_tuples;
    new_tuples.reserve(hash_map.size());
    for (const auto &entry : hash_map) {
        new_tuples.push_back(entry.second);
    }
    t.tuples = new_tuples;
}
