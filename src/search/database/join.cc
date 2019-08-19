#include <algorithm>

#include "join.h"

using namespace std;

void join(Table &t1, Table &t2) {
    /*
     * Join two tables into one.  t1 is the working table and it will be modified
     *
     * We first loop over the parameters of each table and check which indices match.
     * Then, we split it into two cases:
     * 1. If there are no matching indices, we perform the cartesian product of the two tables
     * 2. If at least one parameter matches, we perform a nested loop join.
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
         * If no attribute matches, then we apply a cartesian product
         */
        t1.tuple_index.insert(t1.tuple_index.end(), t2.tuple_index.begin(), t2.tuple_index.end());
        for (const vector<int>& tuple_t1 : t1.tuples) {
            for (const vector<int> &tuple_t2 : t2.tuples) {
                vector<int> aux(tuple_t1);
                aux.insert(aux.end(), tuple_t2.begin(), tuple_t2.end());
                new_tuples.insert(aux);
            }
        }
    }
    else {
        /*
         * Otherwise, we perform the join and the projection
         */


        // We perform a reverse removal procedure so we avoid indexation problems.
        // We also reuse the "to_remove" vector when we are removing elements from the matched arguments.
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

        for (const vector<int> &tuple_t1 : t1.tuples) {
            for (vector<int> tuple_t2 : t2.tuples) {
                bool match = true;
                for (const pair<int, int> &m : matches) {
                    if (tuple_t1[m.first] != tuple_t2[m.second]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    // Code duplicate from the remove above
                    for (int i = to_remove.size()-1; i >= 0 ; --i) {
                        tuple_t2.erase(tuple_t2.begin()+to_remove[i]);
                    }
                    vector<int> aux(tuple_t1);
                    aux.insert(aux.end(), tuple_t2.begin(), tuple_t2.end());
                    new_tuples.insert(aux);
                }
            }
        }
    }
    t1.tuples = new_tuples;
}



