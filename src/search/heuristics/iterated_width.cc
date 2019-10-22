#include <iostream>
#include "iterated_width.h"

using namespace std;



int IteratedWidth::compute_heuristic(const State &s, const Task &task) {
    /*
     *
     * If it is the first time computing a heuristic, preprocess all history
     * container.  Otherwise, simply compute goalcount and check if it is a goal
     * state.  If it is, return 0.  If it is not, compute the novelty based on
     * the scope of the goalcount.

     *
     */

    int h = goalcount.compute_heuristic(s, task);

    if (h == 0) {
        return h;
    }

    for (const auto & relation : s.relations) {
        int index = relation.predicate_symbol;
        for (const auto& tuple : relation.tuples) {
            //pair<vector<unordered_map<vector<int>, int, TupleHash>>::iterator, bool>
            auto it = history[index].insert(make_pair(tuple, h));
            if (it.second) {
                return 1;
            }
            else {
                if (it.first->second > h) {
                    history[index][it.first->first] = h;
                    return 1;
                }
            }
        }
    }

    return 2;
}
