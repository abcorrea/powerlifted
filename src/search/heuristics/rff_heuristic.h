#ifndef SEARCH_HEURISTICS_RFF_HEURISTIC_H_
#define SEARCH_HEURISTICS_RFF_HEURISTIC_H_

#include "heuristic.h"

class RFFHeuristic : public Heuristic {
    int rff_cost;

public:
    RFFHeuristic(const Task &task);

    int compute_heuristic(const DBState &s, const Task &task) override {
        if (task.is_goal(s)) return 0;
        return 1;
    }
};

#endif //SEARCH_HEURISTICS_RFF_HEURISTIC_H_
