
#include "project.h"
#include "table.h"

#include <vector>
#include <unordered_map>

using namespace std;

void project(Table &t, const std::unordered_set<int> &over) {

    vector<int> matches;
    for (int x : over) {
        for (size_t i = 0; i < t.tuple_index.size(); i++) {
            if (x==t.tuple_index[i]) {
                matches.push_back(i);
            }
        }
    }

    unordered_map<vector<int>, vector<int>, TupleHash> hash_map;
    for (const vector<int> &tuple : t.tuples) {
        vector<int> key(matches.size());
        for (size_t i = 0; i < matches.size(); i++) {
            key[i] = tuple[matches[i]];
        }
        if (hash_map.count(key) == 0) {
            hash_map[key] = tuple;
        }
    }

    unordered_set<vector<int>, TupleHash> new_tuples;
    for (const auto& entry: hash_map) {
        new_tuples.insert(entry.second);
    }
    t.tuples = std::move(new_tuples);
}