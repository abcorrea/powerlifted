#ifndef SEARCH_HEURISTICS_LIFTED_HEURISTIC_H_
#define SEARCH_HEURISTICS_LIFTED_HEURISTIC_H_

#include "logic_program.h"

#include "../task.h"

#include "../heuristics/heuristic.h"

class LiftedHeuristic : public Heuristic {
    lifted_heuristic::LogicProgram logic_program;

public:
    LiftedHeuristic(const Task &task, std::ifstream &in);

    int compute_heuristic(const DBState &s, const Task &task) override {
        if (task.is_goal(s)) return 0;
        return 1;
    }
};

#endif //SEARCH_HEURISTICS_LIFTED_HEURISTIC_H_
