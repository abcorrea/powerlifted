#include "heuristic_factory.h"

#include "blind_heuristic.h"
#include "goalcount.h"

#include "../lifted_heuristic/lifted_heuristic.h"

#include <iostream>

#include <boost/algorithm/string.hpp>

Heuristic *HeuristicFactory::create(const std::string &method, const Task &task)
{
    std::cout << "Creating search factory..." << std::endl;
    if (boost::iequals(method, "blind")) {
        return new BlindHeuristic();
    }
    else if (boost::iequals(method, "goalcount")) {
        return new Goalcount();
    }
    else if (boost::iequals(method, "lifted")) {
        return new LiftedHeuristic(task);
    }
    else {
        std::cerr << "Invalid heuristic \"" << method << "\"" << std::endl;
        exit(-1);
    }
}
