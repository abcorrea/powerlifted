
#include "project.h"
#include "table.h"

#include <unordered_set>
#include <vector>

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

    vector<vector<int>> new_tuples;
    for (const vector<int> &tuple : t.tuples) {
        vector<int> key(matches.size());
        for (size_t i = 0; i < matches.size(); i++) {
                key[i] = tuple[matches[i]];
        }
        new_tuples.push_back(std::move(key));
    }

    t.tuples = std::move(new_tuples);
    t.tuple_index = std::move(new_indices);
}
