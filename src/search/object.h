#ifndef SEARCH_OBJECT_H
#define SEARCH_OBJECT_H

#include <string>
#include <utility>
#include <vector>

class Object {

    std::string name;
    int index;
    std::vector<int> types;

public:
    Object(std::string name, int index, std::vector<int> types)
        : name(std::move(name)), index(index), types(std::move(types)) {}

    const std::string &get_name() const { return name; }

    int get_index() const { return index; }

    const std::vector<int> &get_types() const { return types; }

};

#endif // SEARCH_OBJECT_H
