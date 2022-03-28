#ifndef GROUNDER_RULES_H
#define GROUNDER_RULES_H

#include "variable_source.h"

#include "../datalog_atom.h"
#include "../datalog_fact.h"

#include "../annotations/annotation.h"

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
        mapping.clear();
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
    std::unique_ptr<Annotation> annotation;

    VariableSource variable_source;

    static int next_index;

    MapVariablePosition variable_position;

    int get_position_of_atom_in_same_body_rule(int i) const {
        return variable_source.get_position_of_atom_in_same_body_rule(i);
    }


public:
    RuleBase(int weight, DatalogAtom eff, std::vector<DatalogAtom> c, std::unique_ptr<Annotation> annotation) :
        effect(std::move(eff)),
        conditions(std::move(c)),
        weight(weight),
        index(next_index++),
        annotation(std::move(annotation)),
        variable_source(effect, conditions) {
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

    void update_index(int i) {
        index = i;
    }

    // TODO Introduce overload using only index and passing VARIABLE to has_variable
    int get_head_position_of_arg(const Term &arg) const {
        if (variable_position.has_variable(arg))
            return variable_position.at(arg);
        return -1;
    }

    void recreate_map_variable_position(const DatalogAtom &effect) {
        variable_position.create_map(effect);
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

    std::vector<std::pair<int, int>> get_variable_source_table() {
        return variable_source.get_table();
    }

    void update_variable_source_table(VariableSource &&new_source) {
        variable_source = std::move(new_source);
    }

    VariableSource get_variable_source_object() {
        return variable_source;
    }

    const VariableSource &get_variable_source_object_by_ref() const {
        return variable_source;
    }

    void update_conditions(DatalogAtom new_atom,
                           const std::vector<DatalogAtom> &new_rule_conditions,
                           const VariableSource &variable_source_new_rule,
                           std::vector<size_t> &&body_ids);

    void update_single_condition_and_variable_source_table(size_t j, DatalogAtom atom);

    void set_conditions(std::vector<DatalogAtom> new_rule_conditions);

    void replace_single_condition(size_t j, DatalogAtom atom) ;

    void output_variable_table() const;

    std::unique_ptr<Annotation> get_annotation() {
        return std::move(annotation);
    }

    void execute(int head,
                 const Datalog &datalog) const {
        if (annotation) {
            annotation->execute(head, datalog);
        }
    }

    std::vector<int> get_variables_in_body() const {
        std::vector<int> variables;
        for (const DatalogAtom &atom : conditions) {
            for (const Term &term : atom.get_arguments()) {
                if (!term.is_object()) {
                    variables.push_back(term.get_index());
                }
            }
        }
        return variables;
    }

    bool is_equivalent(const RuleBase &other) const {
        return (weight == other.get_weight()) && (get_effect_arguments() == other.get_effect_arguments())  && (conditions == other.get_conditions());
    }

    virtual std::string get_type_name() {
        return "RuleBase";
    }

    void set_specific_condition(size_t i, DatalogAtom atom);

    void update_condition_arguments(int i, std::vector<Term> &terms) {
        conditions[i].update_arguments(terms);
    }

    void update_effect_arguments(std::vector<Term> &terms) {
        effect.update_arguments(terms);
    }
};

}



#endif //GROUNDER_RULES_H
