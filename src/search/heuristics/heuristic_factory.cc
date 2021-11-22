#include "heuristic_factory.h"

#include "blind_heuristic.h"
#include "ff_heuristic.h"
#include "goalcount.h"

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
    else if (boost::iequals(method, "goalcount")) {
        return new Goalcount();
    }
    /*else if (boost::iequals(method, "add")) {
        return new LiftedHeuristic(task, datalog_file, datalog::H_ADD);
    }
    else if (boost::iequals(method, "hmax")) {
        return new LiftedHeuristic(task, datalog_file, datalog::H_MAX);
    }*/
    else if (boost::iequals(method, "ff")) {
        return new FFHeuristic(task);
    }
    else {
        std::cerr << "Invalid heuristic \"" << method << "\"" << std::endl;
        exit(-1);
    }
}
