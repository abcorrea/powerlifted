#include "heuristic_factory.h"

#include "blind_heuristic.h"
#include "add_heuristic.h"
#include "ff_heuristic.h"
#include "goalcount.h"
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
    else if (boost::iequals(method, "goalcount")) {
        return new Goalcount();
    }
    else if (boost::iequals(method, "add")) {
        return new AdditiveHeuristic(task, DatalogTransformationOptions());
    }
    else if (boost::iequals(method, "ff")) {
        return new FFHeuristic(task, DatalogTransformationOptions());
    }
    else if (boost::iequals(method, "ff-norename")) {
        return new FFHeuristic(task, DatalogTransformationOptions(false,true,true));
    }
    else if (boost::iequals(method, "ff-nocollapse")) {
        return new FFHeuristic(task, DatalogTransformationOptions(true,false,true));
    }
    else if (boost::iequals(method, "ff-noremove")) {
        return new FFHeuristic(task, DatalogTransformationOptions(true,true,false));
    }
    else if (boost::iequals(method, "ff-norename-nocollapse")) {
        return new FFHeuristic(task, DatalogTransformationOptions(false,false,true));
    }
    else if (boost::iequals(method, "ff-norename-noremove")) {
        return new FFHeuristic(task, DatalogTransformationOptions(false,true,false));
    }
    else if (boost::iequals(method, "ff-nocollapse-noremove")) {
        return new FFHeuristic(task, DatalogTransformationOptions(true,false,false));
    }
    else if (boost::iequals(method, "ff-norename-nocollapse-noremove")) {
        return new FFHeuristic(task, DatalogTransformationOptions(false,false,false));
    }
    else if (boost::iequals(method, "rff")) {
        return new RFFHeuristic(task, DatalogTransformationOptions());
    }
    else {
        std::cerr << "Invalid heuristic \"" << method << "\"" << std::endl;
        exit(-1);
    }
}
