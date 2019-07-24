//
// Created by gutob on 17.07.2019.
//

#include <unordered_map>
#include <algorithm>
#include "hash_join.h"

using namespace std;

void hash_join(Table &t1, Table &t2) {
    /*
     * This function implements a hash join as follows
     *
     * 1. Star by checking which indexes have the same argument
     * 2. If there is no match, we perform a cartesian product
     * 3. Otherwise, we loop over the first table, create a hash over the
     *    matching keys. Then, loop over the second table searching for hits
     *    in the hash table.
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
         * If no attribute matches, then we apply a cartesian product
         * TODO this code is duplicate from join.cc, make it an auxiliary function
         */
        t1.tuple_index.insert(t1.tuple_index.end(), t2.tuple_index.begin(), t2.tuple_index.end());
        for (const vector<int> &tuple_t1 : t1.tuples) {
            for (const vector<int> &tuple_t2 : t2.tuples) {
                vector<int> aux(tuple_t1);
                aux.insert(aux.end(), tuple_t2.begin(), tuple_t2.end());
                new_tuples.insert(aux);
            }
        }
    }
    else {
        unordered_map<vector<int>, vector<vector<int>>, TupleHash> hash_join_map;
        // Build phase
        for (const vector<int> &tuple : t1.tuples) {
            vector<int> key(matches.size());
            for (int i = 0; i < matches.size(); i++) {
                key[i] = tuple[matches[i].first];
            }
            hash_join_map[key].push_back(tuple);
        }

        // Remove duplicated index. Duplicate code from join.cc
        vector<int> old_indices_t2(t2.tuple_index);
        vector<int> to_remove;
        to_remove.reserve(matches.size());
        for (const pair<int, int> &m : matches) {
            to_remove.push_back(m.second);
        }
        sort(to_remove.begin(), to_remove.end());
        for (int i = to_remove.size()-1; i >= 0 ; --i) {
            t2.tuple_index.erase(t2.tuple_index.begin()+to_remove[i]);
        }
        t1.tuple_index.insert(t1.tuple_index.end(), t2.tuple_index.begin(), t2.tuple_index.end());

        // Probe phase
        for (vector<int> tuple : t2.tuples) {
            vector<int> key(matches.size());
            for (int i = 0; i < matches.size(); i++) {
                key[i] = tuple[matches[i].second];
            }
            if (hash_join_map.count(key) > 0) {
                for (int i = to_remove.size()-1; i >= 0 ; --i) {
                    tuple.erase(tuple.begin()+to_remove[i]);
                }
                for (vector<int> t : hash_join_map[key]) {
                    t.insert(t.end(), tuple.begin(), tuple.end());
                    new_tuples.insert(move(t));
                }

            }
        }

    }
    t1.tuples = new_tuples;

}
