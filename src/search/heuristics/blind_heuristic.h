#ifndef SEARCH_BLIND_HEURISTIC_H
#define SEARCH_BLIND_HEURISTIC_H

#include "heuristic.h"

#include "../task.h"

/**
 * @brief Evaluates all states with h=1. Does not perform goal check.
 *
 * @note Admissible for tasks without zero cost actions.
 *
 */
class BlindHeuristic : public Heuristic {
public:
    int compute_heuristic(const DBState &s, const Task &task) override {
        if (task.is_goal(s)) return 0;
        return 1;
    }
};

#endif //SEARCH_BLIND_HEURISTIC_H
