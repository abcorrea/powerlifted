#ifndef SEARCH_HEURISTICS_HMAX_HEURISTIC_H_
#define SEARCH_HEURISTICS_HMAX_HEURISTIC_H_

#include "heuristic.h"
#include "datalog_transformation_options.h"

#include "../action.h"
#include "../task.h"

#include "../datalog/grounder/weighted_grounder.h"

class HMaxHeuristic : public Heuristic{

    datalog::Datalog datalog;
    datalog::WeightedGrounder grounder;

    datalog::AnnotationGenerator get_annotation_generator();

public:
    HMaxHeuristic(const Task &task) : HMaxHeuristic(task, DatalogTransformationOptions()){};

    HMaxHeuristic(const Task &task, DatalogTransformationOptions opts);

    int compute_heuristic(const DBState &s, const Task &task) override;
};

#endif //SEARCH_HEURISTICS_HMAX_HEURISTIC_H_
