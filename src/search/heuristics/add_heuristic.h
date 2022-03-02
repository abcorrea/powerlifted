#ifndef SEARCH_HEURISTICS_ADD_HEURISTIC_H_
#define SEARCH_HEURISTICS_ADD_HEURISTIC_H_

#include "heuristic.h"

#include "../action.h"
#include "../task.h"

#include "../datalog/grounder/weighted_grounder.h"

class AdditiveHeuristic : public Heuristic{

    datalog::Datalog datalog;
    datalog::WeightedGrounder grounder;

    datalog::AnnotationGenerator get_annotation_generator();

    int state_counter;

public:
    AdditiveHeuristic(const Task &task);

    int compute_heuristic(const DBState &s, const Task &task) override;
};

#endif //SEARCH_HEURISTICS_ADD_HEURISTIC_H_
