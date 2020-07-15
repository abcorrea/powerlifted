#ifndef SEARCH_HEURISTICS_LIFTED_HEURISTIC_H_
#define SEARCH_HEURISTICS_LIFTED_HEURISTIC_H_

#include "logic_program.h"

#include "grounders/weighted_grounder.h"

#include "../task.h"

#include "../heuristics/heuristic.h"

class LiftedHeuristic : public Heuristic {
    lifted_heuristic::LogicProgram logic_program;
    lifted_heuristic::WeightedGrounder grounder;

public:
    LiftedHeuristic(const Task &task, std::ifstream &in);
    ~LiftedHeuristic() = default;

    int compute_heuristic(const DBState &s, const Task &task) final;
};

#endif //SEARCH_HEURISTICS_LIFTED_HEURISTIC_H_
