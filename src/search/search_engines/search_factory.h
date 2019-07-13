#ifndef SEARCH_SEARCH_FACTORY_H
#define SEARCH_SEARCH_FACTORY_H

#include <boost/algorithm/string.hpp>

#include "breadth_first_search.h"
#include "greedy_best_first_search.h"
#include "search.h"

class SearchFactory {
public:
    static Search *new_search_engine(const std::string& method) {
        std::cout << "Creating search factory..." << std::endl;
        if (boost::iequals(method, "naive")) {
            return new BreadthFirstSearch;
        }
        else if (boost::iequals(method, "gbfs")) {
            return new GreedyBestFirstSearch;
        }
        else {
            return nullptr;
        }
    }

};

#endif //SEARCH_SEARCH_FACTORY_H
