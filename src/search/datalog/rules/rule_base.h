#ifndef GROUNDER_RULES_H
#define GROUNDER_RULES_H

#include "../datalog_atom.h"
#include "../datalog_fact.h"

#include <string>
#include <utility>
#include <unordered_set>

namespace datalog {

class MapVariablePosition {
    // Class mapping free variables to positions of the head/effect
    std::unordered_map<Term, int, boost::hash<Term>> mapping;

public:
    MapVariablePosition() = default;

    void create_map(const DatalogAtom &effect) {
        int position_counter = 0;
        for (const auto &eff : effect.get_arguments()) {
            if (!eff.is_object()) {
                // Free variable
                mapping[eff] = position_counter;
            }
            ++position_counter;
        }
    }

    bool has_variable(const Term &t) const {
        return (mapping.count(t) > 0);
    }

    size_t at(const Term &t) const {
        return mapping.at(t);
    }
};

/*
 * Rule: Class implementing the rules of the Datalog program. Divided into
 * three distinct types:
 *
 * Join rules: Binary rules where all vars in the head occur in the body
 * and all variables in the body but not in the head occur in both atoms.
 *
 * Product rules: Special rule for the rules which are not necessarily in
 * any format of the ones above. The goal rule always falls into this case.
 *
 * Project rules: Unary rules where all variables in the head occur in
 * the body.
 *
 */
enum RuleType { GENERIC, JOIN, PRODUCT, PROJECT };

class RuleBase {
protected:
    DatalogAtom effect;
    std::vector<DatalogAtom> conditions;
    int weight;
    int index;
    bool ground_effect;
    static int next_index;

    MapVariablePosition variable_position;

public:
    RuleBase(int weight, DatalogAtom eff, std::vector<DatalogAtom> c) :
        effect(std::move(eff)),
        conditions(std::move(c)),
        weight(weight),
        index(next_index++) {
        variable_position.create_map(effect);
        ground_effect = true;
        for (const auto &e : effect.get_arguments()) {
            if (!e.is_object()) {
                ground_effect = false;
            }
        }
    };

    virtual ~RuleBase() = default;

    virtual void clean_up() = 0;

    bool head_is_ground() const {
        return ground_effect;
    }

    // TODO Introduce overload using only index and passing VARIABLE to has_variable
    int get_head_position_of_arg(const Term &arg) const {
        if (variable_position.has_variable(arg))
            return variable_position.at(arg);
        return -1;
    }

    const DatalogAtom &get_effect() const {
        return effect;
    }

    const std::vector<DatalogAtom> &get_conditions() const {
        return conditions;
    }

    int get_index() const {
        return index;
    }

    int get_weight() const {
        return weight;
    }

    virtual int get_type() const = 0;

    const Arguments &get_condition_arguments(int i) const {
        return conditions[i].get_arguments();
    }

    const Arguments &get_effect_arguments() const {
        return effect.get_arguments();
    }
};

}



#endif //GROUNDER_RULES_H
