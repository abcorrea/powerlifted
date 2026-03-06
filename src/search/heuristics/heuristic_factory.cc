#include "heuristic_factory.h"

#include "add_heuristic.h"
#include "blind_heuristic.h"
#include "ff_heuristic.h"
#include "goalcount.h"
#include "hmax_heuristic.h"
#include "rff_heuristic.h"

#include "datalog_transformation_options.h"

#include <fstream>
#include <iostream>

#include "../utils/string_utils.h"

Heuristic *HeuristicFactory::create(const Options &opt, const Task &task)
{
    const std::string &method = opt.get_evaluator();

    std::cout << "Creating heuristic factory..." << std::endl;

    if (utils::iequals(method, "blind")) {
        return new BlindHeuristic();
    }
    else if (utils::iequals(method, "add")) {
        return new AdditiveHeuristic(task, DatalogTransformationOptions());
    }
    else if (utils::iequals(method, "ff")) {
        return new FFHeuristic(task, DatalogTransformationOptions());
    }
    else if (utils::iequals(method, "goalcount")) {
        return new Goalcount();
    }
    else if (utils::iequals(method, "hmax")) {
        return new HMaxHeuristic(task, DatalogTransformationOptions());
    }
    else if (utils::iequals(method, "rff")) {
        return new RFFHeuristic(task, DatalogTransformationOptions());
    }
    else {
        std::cerr << "Invalid heuristic \"" << method << "\"" << std::endl;
        exit(-1);
    }
}

Heuristic *HeuristicFactory::create_delete_free_heuristic(const std::string &method,
                                                          const Task &task)
{
    if (utils::iequals(method, "add")) {
        return new AdditiveHeuristic(task);
    }
    else if (utils::iequals(method, "ff")) {
        return new FFHeuristic(task);
    }
    else if (utils::iequals(method, "hmax")) {
        return new HMaxHeuristic(task, DatalogTransformationOptions());
    }
    else if (utils::iequals(method, "rff")) {
        return new RFFHeuristic(task);
    }
    else {
        std::cerr << "Invalid delete-free heuristic \"" << method << "\"" << std::endl;
        exit(-1);
    }
}
