#ifndef SEARCH_HEURISTICS_LIFTED_HEURISTIC_H_
#define SEARCH_HEURISTICS_LIFTED_HEURISTIC_H_

#include "heuristic.h"

#include "../task.h"

class LiftedHeuristic : public Heuristic {
public:
    LiftedHeuristic(const Task &task);

    int compute_heuristic(const DBState &s, const Task &task) override {
        if (task.is_goal(s)) return 0;
        return 1;
    }
};

#endif //SEARCH_HEURISTICS_LIFTED_HEURISTIC_H_
