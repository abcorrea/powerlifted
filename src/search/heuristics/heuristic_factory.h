#ifndef SEARCH_HEURISTIC_FACTORY_H
#define SEARCH_HEURISTIC_FACTORY_H

#include "heuristic.h"
#include "blind_heuristic.h"
#include "goalcount.h"

#include <iostream>

#include <boost/algorithm/string.hpp>

/**
 * @brief Factory class to generate corresponding heuristic object
 */
class HeuristicFactory {
public:
    static Heuristic *create(const std::string& method, const Task &task) {
        std::cout << "Creating search factory..." << std::endl;
        if (boost::iequals(method, "blind")) {
            return new BlindHeuristic;
        }
        else if (boost::iequals(method, "goalcount")) {
            return new Goalcount;
        }
        else {
            std::cerr << "Invalid heuristic \"" << method << "\"" << std::endl;
            exit(-1);
        }
    }
};

#endif //SEARCH_HEURISTIC_FACTORY_H
