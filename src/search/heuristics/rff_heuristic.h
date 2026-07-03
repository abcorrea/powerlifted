#ifndef SEARCH_HEURISTICS_RFF_HEURISTIC_H_
#define SEARCH_HEURISTICS_RFF_HEURISTIC_H_

#include "heuristic.h"
#include "datalog_transformation_options.h"

#include "../datalog/grounder/weighted_grounder.h"


class RFFHeuristic : public Heuristic {

    datalog::Datalog datalog;
    datalog::WeightedGrounder grounder;

    int rff_cost;

    datalog::AnnotationGenerator get_annotation_generator();


public:
    RFFHeuristic(const Task &task) : RFFHeuristic(task, DatalogTransformationOptions()){};

    RFFHeuristic(const Task &task, DatalogTransformationOptions opts);

    int compute_heuristic(const DBState &s, const Task &task) override;

    void print_statistics() const override {
        std::cout << "Total atoms produced by grounder: "
                  << grounder.get_cumulative_atoms_produced() << std::endl;
        std::cout << "Total queue pushes by grounder: "
                  << grounder.get_cumulative_queue_pushes() << std::endl;
    }
};

#endif //SEARCH_HEURISTICS_RFF_HEURISTIC_H_
