#include "project.h"

#include <vector>
#include <unordered_set>

using namespace std;

void project(Table &t, const std::unordered_set<int> &over) {

    vector<int> matches(over.size());
    vector<int> new_indices(over.size());
    int counter = 0 ;
    for (int x : over) {
        new_indices[counter] = x;
        for (size_t i = 0; i < t.tuple_index.size(); i++) {
            if (x==t.tuple_index[i])
                matches[counter++] = i;
        }
    }

    unordered_set<vector<int>, TupleHash> new_tuples;
    for (const vector<int> &tuple : t.tuples) {
        vector<int> key(matches.size());
        for (size_t i = 0; i < matches.size(); i++) {
                key[i] = tuple[matches[i]];
        }
        new_tuples.insert(key);
    }

    t.tuples = new_tuples;
    t.tuple_index = new_indices;
}
