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

    int h = goalcount.compute_heuristic(s, task);

    if (h == 0) {
        return h;
    }

    for (const auto & relation : s.relations) {
        int index = relation.predicate_symbol;
        for (const auto& tuple : relation.tuples) {
            if (history[index].count(tuple) == 0 or history[index][tuple] > h) {
                history[index][tuple] = h;
                return 1;
            }
        }
    }

    return 2;
}
