#ifndef SEARCH_HEURISTICS_FF_HEURISTIC_H_
#define SEARCH_HEURISTICS_FF_HEURISTIC_H_

#include "heuristic.h"

#include "../action.h"
#include "../task.h"

#include "../datalog/grounder/weighted_grounder.h"

typedef std::pair<int, std::vector<int>> GroundAction;


class FFHeuristic : public Heuristic{

    datalog::Datalog datalog;
    datalog::WeightedGrounder grounder;

    std::vector<GroundAction> pi_ff;

public:
    FFHeuristic(const Task &task);

    int compute_heuristic(const DBState &s, const Task &task) override;
    datalog::Datalog initialize_datalog(const Task &task);
};

#endif //SEARCH_HEURISTICS_FF_HEURISTIC_H_
