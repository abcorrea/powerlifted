#ifndef SEARCH_ACTION_SCHEMA_H
#define SEARCH_ACTION_SCHEMA_H

#include <string>
#include <utility>
#include <vector>

#include "structures.h"

class ActionSchema {
public:
    explicit ActionSchema(std::string name,
                          int index,
                          int cost,
                          std::vector<Parameter> parameters,
                          std::vector<Atom> precondition,
                          std::vector<Atom> effects) :
                          name(std::move(name)), index(index), cost(cost), parameters(std::move(parameters)),
                          precondition(std::move(precondition)), effects(std::move(effects)) {}

    const std::string &getName() const {
        return name;
    }

    int getIndex() const {
        return index;
    }

    int getCost() const {
        return cost;
    }

    const std::vector<Parameter> &getParameters() const {
        return parameters;
    }

    const std::vector<Atom> &getPrecondition() const {
        return precondition;
    }

    const std::vector<Atom> &getEffects() const {
        return effects;
    }

private:
    std::string name;
    int index;
    int cost;
    std::vector<Parameter> parameters;
    std::vector<Atom> precondition;
    std::vector<Atom> effects;
};


#endif //SEARCH_ACTION_SCHEMA_H
