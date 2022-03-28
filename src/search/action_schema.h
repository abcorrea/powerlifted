#ifndef SEARCH_ACTION_SCHEMA_H
#define SEARCH_ACTION_SCHEMA_H

#include "atom.h"
#include "structures.h"

#include <string>
#include <utility>
#include <vector>

class ActionSchema {

    std::string name;
    int index;
    int cost;
    std::vector<Parameter> parameters;
    std::vector<Atom> precondition;
    std::vector<Atom> effects;
    std::vector<std::pair<int, int>> inequalities;

    /*
     * Nullary predicates are represented as boolean vectors to simplify
     * other parts of the code (e.g., successor generation and action
     * applicability)
     */
    std::vector<bool> positive_nullary_precond;
    std::vector<bool> negative_nullary_precond;
    std::vector<bool> positive_nullary_effects;
    std::vector<bool> negative_nullary_effects;

public:
    explicit ActionSchema(std::string name,
                          int index,
                          int cost,
                          std::vector<Parameter> parameters,
                          std::vector<Atom> precondition,
                          std::vector<Atom> effects,
                          std::vector<std::pair<int, int>> inequalities,
                          std::vector<bool> positive_nullary_precond,
                          std::vector<bool> negative_nullary_precond,
                          std::vector<bool> positive_nullary_effects,
                          std::vector<bool> negative_nullary_effects);

    const std::string &get_name() const {
        return name;
    }

    int get_index() const {
        return index;
    }

    int get_cost() const {
        return cost;
    }

    const std::vector<Parameter> &get_parameters() const {
        return parameters;
    }

    const std::vector<Atom> &get_precondition() const {
        return precondition;
    }

    const std::vector<Atom> &get_effects() const {
        return effects;
    }

    const std::vector<std::pair<int, int>> &get_inequalities() const {
        return inequalities;
    }

    const std::vector<bool> &get_positive_nullary_precond() const {
        return positive_nullary_precond;
    }

    const std::vector<bool> &get_negative_nullary_precond() const {
        return negative_nullary_precond;
    }

    const std::vector<bool> &get_positive_nullary_effects() const {
        return positive_nullary_effects;
    }

    const std::vector<bool> &get_negative_nullary_effects() const {
        return negative_nullary_effects;
    }

    bool is_ground() const {
        return parameters.empty();
    }

};

#endif //SEARCH_ACTION_SCHEMA_H
