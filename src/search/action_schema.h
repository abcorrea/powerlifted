#ifndef SEARCH_ACTION_SCHEMA_H
#define SEARCH_ACTION_SCHEMA_H

#include "atom.h"
#include "structures.h"

#include <string>
#include <utility>
#include <vector>

/**
 * @brief A universally quantified conditional effect of an action schema:
 * for all instantiations of the quantified variables satisfying the
 * condition (in the state where the action is applied), apply the effects.
 *
 * Variables are indexed like action parameters: indices below the index of
 * the first quantified variable refer to the action parameters (or fresh
 * variables), the quantified variables follow. Every quantified variable
 * occurs in some positive condition atom (the translator adds type atoms).
 *
 * @var parameters: quantified variables.
 * @var condition: non-nullary, non-'=' condition literals (positive and
 * negated).
 * @var equalities: '=' condition literals (equalities and inequalities).
 * @var positive_nullary_condition, negative_nullary_condition: nullary
 * condition atoms (predicate indices).
 * @var effects: non-nullary effect literals.
 * @var positive_nullary_effects, negative_nullary_effects: nullary effects.
 */
struct ConditionalEffect {
    std::vector<Parameter> parameters;
    std::vector<Atom> condition;
    std::vector<Atom> equalities;
    std::vector<int> positive_nullary_condition;
    std::vector<int> negative_nullary_condition;
    std::vector<Atom> effects;
    std::vector<int> positive_nullary_effects;
    std::vector<int> negative_nullary_effects;
};

class ActionSchema {

    std::string name;
    int index;
    int cost;
    std::vector<Parameter> parameters;
    //! The first num_external_parameters parameters are the parameters of
    //! the PDDL action schema; the rest were introduced by the translator
    //! (compiled-away existential quantifiers) and are not part of the
    //! ground action name in plans.
    int num_external_parameters;
    std::vector<FreshVariable> fresh_variables;
    std::vector<Atom> precondition;
    std::vector<Atom> effects;
    std::vector<Atom> static_precondition;

    /*
     * Nullary predicates are represented as boolean vectors to simplify
     * other parts of the code (e.g., successor generation and action
     * applicability)
     */
    std::vector<bool> positive_nullary_precond;
    std::vector<bool> negative_nullary_precond;
    std::vector<bool> positive_nullary_effects;
    std::vector<bool> negative_nullary_effects;

    std::vector<ConditionalEffect> conditional_effects;

public:
    explicit ActionSchema(std::string name,
                          int index,
                          int cost,
                          std::vector<Parameter> parameters,
                          int num_external_parameters,
                          std::vector<FreshVariable> fresh_variables,
                          std::vector<Atom> precondition,
                          std::vector<Atom> effects,
                          std::vector<Atom> static_precondition,
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

    int get_num_external_parameters() const {
        return num_external_parameters;
    }

    const std::vector<FreshVariable> &get_fresh_variables() const {
        return fresh_variables;
    }

    const std::vector<Atom> &get_precondition() const {
        return precondition;
    }

    const std::vector<Atom> &get_effects() const {
        return effects;
    }

    const std::vector<Atom> &get_static_precondition() const {
        return static_precondition;
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

    const std::vector<ConditionalEffect> &get_conditional_effects() const {
        return conditional_effects;
    }

    bool has_conditional_effects() const {
        return !conditional_effects.empty();
    }

    void set_conditional_effects(std::vector<ConditionalEffect> &&ces) {
        conditional_effects = std::move(ces);
    }

    bool is_ground() const {
        return parameters.empty();
    }

    bool operator<(const ActionSchema& other) const {
        return index < other.index;
    }

    bool operator==(const ActionSchema& other) const {
        return index == other.index;
    }

    bool operator!=(const ActionSchema& other) const {
        return index != other.index;
    }
};

namespace std {
template <>
struct hash<ActionSchema> {
    size_t operator()(const ActionSchema &action) const {
        return hash<int>()(action.get_index());
    }
};
}  // namespace std

#endif //SEARCH_ACTION_SCHEMA_H
