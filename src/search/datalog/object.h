#ifndef GROUNDER__OBJECT_H_
#define GROUNDER__OBJECT_H_

#include <string>
#include <utility>
#include <vector>

namespace datalog {

/*
 * This class represents an object (i.e., constant) of the task and the logic
 * program. Every object has a name and a unique index.
 *
 */
class Object {
    static int next_index;
    int index = -1;
    std::string name;

public:

    explicit Object(std::string name) : name(std::move(name)) {
        index = next_index++;
    };

    const std::string &get_name() const {
        return name;
    }

};

}

#endif //GROUNDER__OBJECT_H_
