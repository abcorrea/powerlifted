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
                          std::vector<Atom> effects,
                          std::vector<std::pair<int,int>> inequalities,
                          std::vector<bool> positive_nullary_precond,
                          std::vector<bool> negative_nullary_precond,
                          std::vector<bool> positive_nullary_effects,
                          std::vector<bool> negative_nullary_effects);

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

    const std::vector<std::pair<int, int>> &getInequalities() const {
        return inequalities;
    }


    std::vector<bool> positive_nullary_precond;
    std::vector<bool> negative_nullary_precond;
    std::vector<bool> positive_nullary_effects;
    std::vector<bool> negative_nullary_effects;

private:
    std::string name;
    int index;
    int cost;
    std::vector<Parameter> parameters;
    std::vector<Atom> precondition;
    std::vector<Atom> effects;
    std::vector<std::pair<int,int>> inequalities;
};


#endif //SEARCH_ACTION_SCHEMA_H
