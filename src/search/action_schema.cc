#include "action_schema.h"

#include <utility>

ActionSchema::ActionSchema(std::string name, int index, int cost, std::vector<Parameter> parameters,
                           std::vector<Atom> precondition, std::vector<Atom> effects,
                           std::vector<std::pair<int, int>> inequalities,
                           std::vector<bool> positive_nullary_precond,
                           std::vector<bool> negative_nullary_precond,
                           std::vector<bool> positive_nullary_effects,
                           std::vector<bool> negative_nullary_effects) :
        name(std::move(name)), index(index), cost(cost), parameters(std::move(parameters)),
        precondition(std::move(precondition)), effects(std::move(effects)),
        inequalities(std::move(inequalities)),
        positive_nullary_precond(std::move(positive_nullary_precond)),
        negative_nullary_precond(std::move(negative_nullary_precond)),
        positive_nullary_effects(std::move(positive_nullary_effects)),
        negative_nullary_effects(std::move(negative_nullary_effects)) {}

