//
// Created by gutob on 13.07.2019.
//

#include <iostream>
#include "iterated_width.h"

using namespace std;

int IteratedWidth::compute_heuristic(const State &s, const Task &task) {
    if (first_time) {
        // Initialize IW structures
        history.clear();
        history.reserve(s.relations.size());
        for (int i = 0; i < s.relations.size(); ++i) {
            history.emplace_back();
        }
        first_time = false;
    }

    if (task.is_goal(s, task.goal)) {
        return 0;
    }

    bool found = false;
    for (const auto & relation : s.relations) {
        int index = relation.predicate_symbol;
        for (auto tuple : relation.tuples) {
            if (history[index].count(tuple) == 0) {
                found = true;
            }
            history[index].emplace(tuple);
            if (found) {
                return 1;
            }
        }
    }

    return 2;
}
