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
    std::ifstream datalog_file(opt.get_datalog_file());
    if (!datalog_file and ((opt.get_evaluator() == "add") or (opt.get_evaluator() == "hmax"))) {
        std::cerr << "Error opening the Datalog model file: " << opt.get_datalog_file() << std::endl;
        exit(-1);
    }
    std::cout << "Creating search factory..." << std::endl;
    if (boost::iequals(method, "blind")) {
        return new BlindHeuristic();
    }
    else if (boost::iequals(method, "goalcount")) {
        return new Goalcount();
    }
    else if (boost::iequals(method, "add")) {
        return new LiftedHeuristic(task, datalog_file, lifted_heuristic::H_ADD);
    }
    else if (boost::iequals(method, "hmax")) {
        return new LiftedHeuristic(task, datalog_file, lifted_heuristic::H_MAX);
    }
    else {
        std::cerr << "Invalid heuristic \"" << method << "\"" << std::endl;
        exit(-1);
    }
}
