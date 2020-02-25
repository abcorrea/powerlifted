#ifndef SEARCH_BLIND_HEURISTIC_H
#define SEARCH_BLIND_HEURISTIC_H

#include "heuristic.h"

/**
 * @brief Evaluates all states with h=1. Does not perform goal check.
 *
 * @note Admissible for tasks without zero cost actions.
 *
 */
class BlindHeuristic : public Heuristic {
public:
    int compute_heuristic(const State &s, const Task &task) override {
        return 1;
    }
};

#endif //SEARCH_BLIND_HEURISTIC_H
