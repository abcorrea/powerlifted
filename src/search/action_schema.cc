#include <utility>

#include <utility>

#include "action_schema.h"

ActionSchema::ActionSchema(std::string name, int index, int cost, std::vector<Parameter> parameters,
                           std::vector<Atom> precondition, std::vector<Atom> effects,
                           std::vector<std::pair<int, int>> inequalities) :
        name(std::move(name)), index(index), cost(cost), parameters(std::move(parameters)),
        precondition(std::move(precondition)), effects(std::move(effects)),
        inequalities(std::move(inequalities)) {}
