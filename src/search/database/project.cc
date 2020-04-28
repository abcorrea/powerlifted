
#include "project.h"
#include "table.h"

#include <vector>

using namespace std;

void project(Table &t, const std::unordered_set<int> &over) {

    vector<int> matches;
    for (int x : over)
        for (size_t i = 0; i < t.tuple_index.size(); i++)
            if (x == t.tuple_index[i])
                matches.push_back(i);

    unordered_set<vector<int>, TupleHash> projected_tuples;
    for (const vector<int> &tuple : t.tuples) {
        vector<int> key(matches.size());
        for (size_t i = 0; i < matches.size(); i++) {
                key[i] = tuple[matches[i]];
            }
        projected_tuples.insert(tuple);
    }

    unordered_set<vector<int>, TupleHash> new_tuples;
    for (const auto& tup: projected_tuples) {
        new_tuples.insert(tup);
    }
    t.tuples = std::move(new_tuples);
}