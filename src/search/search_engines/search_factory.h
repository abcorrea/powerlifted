#ifndef SEARCH_SEARCH_FACTORY_H
#define SEARCH_SEARCH_FACTORY_H

#include <boost/algorithm/string.hpp>

#include "search.h"
#include "breadth_first_search.h"

class SearchFactory {
public:
    static Search *new_search_engine(const std::string& method) {
        std::cout << "Creating search factory..." << std::endl;
        if (boost::iequals(method, "naive")) {
            return new BreadthFirstSearch;
        }
        else {
            return nullptr;
        }
    }

};

#endif //SEARCH_SEARCH_FACTORY_H
