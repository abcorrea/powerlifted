#ifndef SEARCH_HEURISTICS_UTILS_H_
#define SEARCH_HEURISTICS_UTILS_H_

#include "datalog_transformation_options.h"

#include "../task.h"

#include "../datalog/grounder/weighted_grounder.h"

// grounder_type is the cost aggregation (datalog::H_ADD or datalog::H_MAX) of
// the grounder that will evaluate the program; the static-stratum
// materialization uses the same aggregation.
datalog::Datalog initialize_datalog(const Task &task,
                                    datalog::AnnotationGenerator annotation_generator,
                                    const DatalogTransformationOptions &opts,
                                    int grounder_type = datalog::H_ADD);

std::vector<datalog::Fact> get_datalog_facts_from_state(const DBState &s, const Task &task);

#endif //SEARCH_HEURISTICS_UTILS_H_
