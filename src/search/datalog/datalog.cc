#include "datalog.h"

#include "rules/generic_rule.h"

#include <iostream>
#include <memory>

using namespace datalog;
using namespace std;

Datalog::Datalog(const Task &task) : task(task) {

    // Idea: pass callback function as parameter to handle annotations
    create_rules();

    for (const auto &rule : rules) {
        output_rule(rule);
    }
}

void Datalog::get_nullary_atoms_from_vector(const vector<bool> &nullary_predicates_in_precond,
                                            vector<size_t> &nullary_preconds) const {
    for (size_t i = 0; i < nullary_predicates_in_precond.size(); ++i) {
        if (nullary_predicates_in_precond[i]) {
            nullary_preconds.push_back(i);
        }
    }
}

void Datalog::create_rules() {
    for (const ActionSchema &schema : task.get_action_schemas()) {
        const std::vector<bool> &nullary_predicates_in_precond = schema.get_positive_nullary_precond();
        std::vector<size_t> nullary_preconds;
        get_nullary_atoms_from_vector(nullary_predicates_in_precond, nullary_preconds);
        generate_action_rule(schema, nullary_preconds);
        generate_action_effect_rules(schema);
        //generate_rules_with_n_ary_heads(schema, nullary_preconds);
        //generate_rules_with_nullary_heads(schema, nullary_preconds);
    }
}

void Datalog::generate_action_rule(const ActionSchema &schema,
                                   std::vector<size_t> nullary_preconds) {
    string action_predicate = "action-" + schema.get_name();
    size_t idx = new_predicates.size();
    map_new_predicates_to_idx.emplace(action_predicate, idx);
    DatalogAtom eff(schema, idx);
    new_predicates.emplace_back(action_predicate);
    vector<DatalogAtom> body = get_atoms_in_rule_body(schema, nullary_preconds);
    rules.emplace_back(make_unique<GenericRule>(schema.get_cost(), eff, move(body)));
}

void Datalog::generate_action_effect_rules(const ActionSchema &schema) {
    vector<DatalogAtom> body = get_action_effect_rule_body(schema);
    for (const Atom &eff : schema.get_effects()) {
        if (eff.is_negated())
            continue;
        DatalogAtom effect(eff);
        rules.emplace_back(make_unique<GenericRule>(schema.get_cost(), eff, body));
    }
    const vector<bool> &nullary_predicates_in_eff = schema.get_positive_nullary_effects();
    vector<size_t> nullary_effects;
    get_nullary_atoms_from_vector(nullary_predicates_in_eff, nullary_effects);
    for (size_t eff_idx : nullary_effects) {
        DatalogAtom eff(Arguments(), eff_idx, false);
        rules.emplace_back(make_unique<GenericRule>(schema.get_cost(), eff, body));
    }
}

vector<DatalogAtom> Datalog::get_action_effect_rule_body(const ActionSchema &schema) {
    vector<DatalogAtom> body(1);
    string action_predicate = "action-" + schema.get_name();
    size_t idx = map_new_predicates_to_idx[action_predicate];
    body[0] = DatalogAtom(schema, idx);
    return body;
}


void Datalog::generate_rules_with_n_ary_heads(const ActionSchema &schema,
                                              const vector<size_t> &nullary_preconds) {
    for (const Atom &eff : schema.get_effects()) {
        if (eff.is_negated())
            continue;
        DatalogAtom effect(eff);
        vector<DatalogAtom> body = get_atoms_in_rule_body(schema, nullary_preconds);
        rules.emplace_back(make_unique<GenericRule>(schema.get_cost(), eff, move(body)));
    }
}

void Datalog::generate_rules_with_nullary_heads(const ActionSchema &schema,
                                                const vector<size_t> &nullary_preconds) {
    const vector<bool> &nullary_predicates_in_eff = schema.get_positive_nullary_effects();
    vector<size_t> nullary_effects;
    get_nullary_atoms_from_vector(nullary_predicates_in_eff, nullary_effects);
    for (size_t eff_idx : nullary_effects) {
        DatalogAtom eff(Arguments(), eff_idx, false);
        vector<DatalogAtom> body = get_atoms_in_rule_body(schema, nullary_preconds);
        rules.emplace_back(make_unique<GenericRule>(schema.get_cost(), eff, move(body)));
    }
}

vector<DatalogAtom> Datalog::get_atoms_in_rule_body(const ActionSchema &schema,
                                                    const vector<size_t> &nullary_preconds) const {
    vector<DatalogAtom> body;
    for (const Atom &condition : schema.get_precondition()) {
        if (condition.is_negated())
            continue;
        body.emplace_back(DatalogAtom(condition));
    }
    for (size_t nullary_idx : nullary_preconds) {
        body.emplace_back(DatalogAtom(Arguments(), nullary_idx, false));
    }
    return body;
}

void Datalog::output_rule(const std::unique_ptr<RuleBase> &rule) {
    DatalogAtom effect = rule->get_effect();
    output_atom(effect);
    cout << " :- ";
    size_t number_conditions = rule->get_conditions().size();
    for (const auto &condition : rule->get_conditions()) {
        --number_conditions;
        output_atom(condition);
        if (number_conditions > 0) {
            cout << ", ";
        }
        else {
            cout << "." << endl;
        }
    }
}

void Datalog::output_atom(const DatalogAtom &atom) {
    if (atom.is_pred_symbol_new()) {
        assert(int(new_predicates.size()) > atom.get_predicate_index());
        std::cout << new_predicates[atom.get_predicate_index()];
    }
    else {
        cout << task.get_predicate_name(atom.get_predicate_index());
    }
    output_parameters(atom.get_arguments());
}

void Datalog::output_parameters(const Arguments& v) {
    cout << '(';
    int number_params = v.size();
    for (auto arg : v) {
        if (arg.is_object()) {
            cout << task.get_object_name(arg.get_index());
        } else {
            cout << "?v" << arg.get_index();
        }
        if (--number_params > 0) cout << ", ";
    }
    cout << ')';
}