#ifndef SEARCH_SEARCH_FACTORY_H
#define SEARCH_SEARCH_FACTORY_H

#include <string>

class SearchBase;

class SearchFactory {
public:
    static SearchBase*create(const std::string& method, const std::string& state_type);
};

#endif //SEARCH_SEARCH_FACTORY_H
