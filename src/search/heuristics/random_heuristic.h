#ifndef SEARCH_HEURISTICS_RANDOM_HEURISTIC_H_
#define SEARCH_HEURISTICS_RANDOM_HEURISTIC_H_


#include "heuristic.h"

#include "../task.h"
#include "../datalog/datalog.h"
#include "datalog_transformation_options.h"
#include "../datalog/grounder/weighted_grounder.h"
#include "../datalog/annotations/annotation.h"

class RandomHeuristic : public Heuristic{
/**
 * @brief Evaluates all states with h=1. Does not perform goal check. 
 * For simply test heuritic intergration, copy from blind heuristic.
 *
 * @note Admissible for tasks without zero cost actions.
 *
 */
    datalog::Datalog datalog;
    datalog::WeightedGrounder grounder;

    datalog::AnnotationGenerator get_annotation_generator();



public:
    RandomHeuristic(const Task &task) : RandomHeuristic(task, DatalogTransformationOptions()){};

    RandomHeuristic(const Task &task, DatalogTransformationOptions opts);

    int compute_heuristic(const DBState &s, const Task &task)override;

};

#endif //SEARCH_HEURISTICS_RANDOM_HEURISTIC_H_
