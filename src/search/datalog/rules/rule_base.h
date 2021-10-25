#ifndef GROUNDER_RULES_H
#define GROUNDER_RULES_H

#include "../datalog_atom.h"
#include "../datalog_fact.h"

#include <string>
#include <utility>
#include <unordered_set>

namespace datalog {

class MapVariablePosition {
    // Class mapping free variables to positions of the head/effect.
    // This class is used only for the join/product/projection operation, and not to retrieve
    // the full instantiation of action by "unsplitting" it.
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

    // TODO Rewrite it.
    // Where to get the variable from:
    // Entry i is the ith variable of the action schema.
    // If the first element is negative, then it indicates the atom in the body where this variable
    // can be retrieved. If its positive, it indicates the auxiliary body atom that can query this
    // variable. The second element indicates the position of the variable in the rule atom or in the
    // auxiliary atom variables.
    // Attention: if it is negative, we need to reduce 1 after transforming to positive (to avoid ambiguity with 0)
    std::vector<std::pair<int, int>> variable_source;

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
        for (const Term &arg : effect.get_arguments()) {
            if (arg.is_object()) continue;
            std::pair<int, int> p;
            bool found_correspondence = false;
            int body_atom_counter = 0;
            for (const DatalogAtom &b : conditions) {
                int position_counter = 0;
                for (const Term &body_arg : b.get_arguments()) {
                    if (body_arg.is_object()) continue;
                    if (body_arg.get_index() == arg.get_index()) {
                        p.first = (body_atom_counter + 1) * -1;
                        p.second = position_counter;
                        found_correspondence = true;
                        break;
                    }
                    position_counter++;
                }
                if (found_correspondence) break;
                body_atom_counter++;
            }
            variable_source.emplace_back(p);
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

    int get_atom_position_same_rule_body(int i) const {
        if (i < 0)
            return (i*-1)-1;
        else
            return i;
    }

    void output_variable_table() {
        int v = 0;
        for (const std::pair<int, int> p : variable_source) {
            bool is_found_in_rule = (p.first < 0);
            int position_in_body = p.second;
            int body_position = get_atom_position_same_rule_body(p.first);
            if (is_found_in_rule) {
                std::cout << "?v" << v << ' ' << "BODY " << body_position << ' ' << position_in_body << std::endl;
            }
            else {
                std::cout << "?v" << v << ' ' << "BODY " << body_position << ' ' << position_in_body << std::endl;
            }
            ++v;
        }
    }
};

}



#endif //GROUNDER_RULES_H
