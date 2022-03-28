#include "heuristic_factory.h"

#include "add_heuristic.h"
#include "blind_heuristic.h"
#include "ff_heuristic.h"
#include "goalcount.h"
#include "hmax_heuristic.h"
#include "rff_heuristic.h"

#include "datalog_transformation_options.h"

#include <iostream>
#include <fstream>

#include <boost/algorithm/string.hpp>

Heuristic *HeuristicFactory::create(const Options &opt, const Task &task)
{
    const std::string& method = opt.get_evaluator();
    std::ifstream datalog_file(opt.get_datalog_file());
    std::cout << "Creating search factory..." << std::endl;
    if (boost::iequals(method, "blind")) {
        return new BlindHeuristic();
    }
    else if (boost::iequals(method, "add")) {
        return new AdditiveHeuristic(task, DatalogTransformationOptions());
    }
    else if (boost::iequals(method, "ff")) {
        return new FFHeuristic(task, DatalogTransformationOptions());
    }
    else if (boost::iequals(method, "goalcount")) {
        return new Goalcount();
    }
    else if (boost::iequals(method, "hmax")) {
        return new HMaxHeuristic(task, DatalogTransformationOptions());
    }
    else if (boost::iequals(method, "rff")) {
        return new RFFHeuristic(task, DatalogTransformationOptions());
    }
    else {
        std::cerr << "Invalid heuristic \"" << method << "\"" << std::endl;
        exit(-1);
    }
}

Heuristic *HeuristicFactory::create_delete_free_heuristic(const std::string &method, const Task &task)
{
    if (boost::iequals(method, "add")) {
        return new AdditiveHeuristic(task);
    }
    else if (boost::iequals(method, "ff")) {
        return new FFHeuristic(task);
    }
    else if (boost::iequals(method, "hmax")) {
        return new HMaxHeuristic(task, DatalogTransformationOptions());
    }
    else if (boost::iequals(method, "rff")) {
        return new RFFHeuristic(task);
    }
    else {
        std::cerr << "Invalid delete-free heuristic \"" << method << "\"" << std::endl;
        exit(-1);
    }
}
