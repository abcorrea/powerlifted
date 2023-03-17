#include "action_schema.h"

#include <utility>

ActionSchema::ActionSchema(std::string name, int index, int cost, std::vector<Parameter> parameters,
                           std::vector<Atom> precondition, std::vector<Atom> effects,
                           std::vector<Atom> static_precondition,
                           std::vector<bool> positive_nullary_precond,
                           std::vector<bool> negative_nullary_precond,
                           std::vector<bool> positive_nullary_effects,
                           std::vector<bool> negative_nullary_effects) :
        name(std::move(name)), index(index), cost(cost), parameters(std::move(parameters)),
        precondition(std::move(precondition)), effects(std::move(effects)),
        static_precondition(std::move(static_precondition)),
        positive_nullary_precond(std::move(positive_nullary_precond)),
        negative_nullary_precond(std::move(negative_nullary_precond)),
        positive_nullary_effects(std::move(positive_nullary_effects)),
        negative_nullary_effects(std::move(negative_nullary_effects)) {}

