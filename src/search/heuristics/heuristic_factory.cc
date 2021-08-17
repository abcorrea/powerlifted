#include "heuristic_factory.h"

#include "blind_heuristic.h"
#include "goalcount.h"

#include "../lifted_heuristic/lifted_heuristic.h"

#include <iostream>
#include <fstream>

#include <boost/algorithm/string.hpp>

Heuristic *HeuristicFactory::create(const Options &opt, const Task &task)
{
    const std::string& method = opt.get_evaluator();
    std::cout << "Creating search factory..." << std::endl;
    if (boost::iequals(method, "blind")) {
        return new BlindHeuristic();
    }
    else if (boost::iequals(method, "goalcount")) {
        return new Goalcount();
    }
    else if (boost::iequals(method, "add")) {
        return new LiftedHeuristic(task, opt.get_datalog_file(), lifted_heuristic::H_ADD);
    }
    else if (boost::iequals(method, "hmax")) {
        return new LiftedHeuristic(task, opt.get_datalog_file(), lifted_heuristic::H_MAX);
    }
    else if (boost::iequals(method, "rff")) {
        return new LiftedHeuristic(task, opt.get_datalog_file(), lifted_heuristic::RFF);
    }
    else {
        std::cerr << "Invalid heuristic \"" << method << "\"" << std::endl;
        exit(-1);
    }
}
