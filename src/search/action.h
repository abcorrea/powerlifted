#ifndef SEARCH_ACTION_H
#define SEARCH_ACTION_H

#include <utility>
#include <vector>

/*
 * The Action class represents a grounded action.
 * The attribute index is the action schema index, from where we can retrieve its name and number of parameters.
 * The attribute instantiation is a list of object indices that instantiate each argument of the corresponding action
 * schema, in order as they appear in the action schema.
 */

class Action {
public:
    int index;
    std::vector<int> instantiation;

    explicit Action(int index, std::vector<int> &&instantiation) : index(index),
                                                                 instantiation(std::move(instantiation)) {}

    Action() = default;

};

#endif //SEARCH_ACTION_H
