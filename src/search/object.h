#ifndef SEARCH_OBJECT_H
#define SEARCH_OBJECT_H

#include <string>
#include <utility>
#include <vector>

class Object {
public:
    Object(std::string name, int index, std::vector<int> types) : name(std::move(name)), index(index),
                                                                  types(std::move(types)) {
    }

    const std::string &getName() const {
        return name;
    }

    int getIndex() const {
        return index;
    }

    const std::vector<int> &getTypes() const {
        return types;
    }

private:
    std::string name;
    int index;
    std::vector<int> types;
};


#endif //SEARCH_OBJECT_H
