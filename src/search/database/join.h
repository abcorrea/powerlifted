#ifndef SEARCH_JOIN_H
#define SEARCH_JOIN_H

#include <algorithm>
#include <utility>
#include <vector>

#include "table.h"

using namespace std;

Table join(Table t1, Table &t2) {
    /*
     * Join two tables into one.
     *
     * We first loop over the parameters of each table and check which indices match.
     * Then, we split it into two cases:
     * 1. If there are no matching indices, we perform the cartesian product of the two tables
     * 2. If at least one parameter matches, we perform a nested loop join.
     *
     */

    vector<vector<int>> new_tuples;
    vector<int> new_indices;
    vector<pair<int, int>> matches;
    for (int i = 0; i < t1.tuple_index.size(); ++i) {
        for (int j = 0; j < t2.tuple_index.size(); ++j) {
            if (t1.tuple_index[i] == t2.tuple_index[j])
                matches.emplace_back(i, j);
        }
    }

    if (matches.empty()) {
        /*
         * If no attribute matches, then we apply a cartesian product
         */
        new_indices = vector<int>(t1.tuple_index);
        new_indices.insert(new_indices.end(), t2.tuple_index.begin(), t2.tuple_index.end());
        for (const vector<int>& tuple_t1 : t1.tuples) {
            for (vector<int> tuple_t2 : t2.tuples) {
                vector<int> aux(tuple_t1);
                aux.insert(aux.end(), tuple_t2.begin(), tuple_t2.end());
                new_tuples.push_back(aux);
            }
        }
    }
    else {
        /*
         * Otherwise, we perform the join and the projection
         */
        new_indices = vector<int>(t1.tuple_index);
        vector<int> aux_indices(t2.tuple_index);

        /*
         * We perform a reverse removal procedure so we avoid indexation problems.
         * We also reuse the "to_remove" vector when we are removing elements from the matched tuples.
         */
        vector<int> to_remove;
        to_remove.reserve(matches.size());

        for (pair<int, int> m : matches) {
            to_remove.push_back(m.second);
        }
        sort(to_remove.begin(), to_remove.end());
        for (int i = to_remove.size()-1; i >= 0 ; --i) {
            aux_indices.erase(aux_indices.begin()+to_remove[i]);
        }
        new_indices.insert(new_indices.end(), aux_indices.begin(), aux_indices.end());

        for (const vector<int> &tuple_t1 : t1.tuples) {
            for (vector<int> tuple_t2 : t2.tuples) {
                bool match = true;
                for (pair<int, int> m : matches) {
                    if (tuple_t1[m.first] != tuple_t2[m.second]) {
                         match = false;
                         break;
                    }
                }
                if (match) {
                    vector<int> aux(tuple_t1);
                    vector<int> aux2(tuple_t2);
                    // Code duplicate from the remove above
                    for (int i = to_remove.size()-1; i >= 0 ; --i) {
                        aux2.erase(aux2.begin()+to_remove[i]);
                    }
                    aux.insert(aux.end(), aux2.begin(), aux2.end());
                    new_tuples.push_back(aux);
                }
            }
        }
    }

    return Table(new_tuples, new_indices);
}


#endif //SEARCH_JOIN_H
