#ifndef SEARCH_HEURISTICS_FF_HEURISTIC_H_
#define SEARCH_HEURISTICS_FF_HEURISTIC_H_

#include "heuristic.h"
#include "datalog_transformation_options.h"

#include "../action.h"
#include "../task.h"

#include "../datalog/grounder/weighted_grounder.h"

typedef std::pair<int, std::vector<int>> GroundAction;


class FFHeuristic : public Heuristic{

    datalog::Datalog datalog;
    datalog::WeightedGrounder grounder;

    std::vector<GroundAction> pi_ff;

    datalog::AnnotationGenerator get_annotation_generator();

public:
    FFHeuristic(const Task &task) : FFHeuristic(task, DatalogTransformationOptions()) {}

    FFHeuristic(const Task &task, DatalogTransformationOptions opts);

    int compute_heuristic(const DBState &s, const Task &task) override;
};

#endif //SEARCH_HEURISTICS_FF_HEURISTIC_H_
