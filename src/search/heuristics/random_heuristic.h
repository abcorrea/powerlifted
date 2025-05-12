#ifndef SEARCH_HEURISTICS_RANDOM_HEURISTIC_H_
#define SEARCH_HEURISTICS_RANDOM_HEURISTIC_H_


#include "heuristic.h"

#include "../task.h"


class RandomHeuristic : public Heuristic{
/**
 * @brief Evaluates all states with h=1. Does not perform goal check. 
 * For simply test heuritic intergration, copy from blind heuristic.
 *
 * @note Admissible for tasks without zero cost actions.
 *
 */

public:
    int compute_heuristic(const DBState &s, const Task &task) override{
        if (task.is_goal(s)) return 0;
        return 1;
    };
};

#endif //SEARCH_HEURISTICS_RANDOM_HEURISTIC_H_
