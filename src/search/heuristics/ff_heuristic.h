#ifndef SEARCH_HEURISTICS_FF_HEURISTIC_H_
#define SEARCH_HEURISTICS_FF_HEURISTIC_H_

#include "heuristic.h"

#include "../action.h"
#include "../task.h"


typedef std::pair<int, std::vector<int>> GroundAction;


class FFHeuristic : public Heuristic{
    std::vector<GroundAction> pi_ff;

public:
    FFHeuristic(const Task &task);

    int compute_heuristic(const DBState &s, const Task &task) override {
        if (task.is_goal(s)) return 0;
        return 1;
    }
};

#endif //SEARCH_HEURISTICS_FF_HEURISTIC_H_
