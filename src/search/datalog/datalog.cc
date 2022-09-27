#include "datalog.h"

#include "rules/generic_rule.h"
#include "rules/product.h"

#include "transformations/action_predicate_removal.h"
#include "transformations/generate_edb.h"
#include "transformations/goal_rule.h"
#include "transformations/normal_form.h"
#include "transformations/remove_equivalent_rules.h"
#include "transformations/variable_renaming.h"

#include <iostream>
#include <memory>
#include <stack>

using namespace datalog;
using namespace std;

Datalog::Datalog(const Task &task, AnnotationGenerator annotation_generator) : task(task) {

    for (auto p : task.predicates) {
        predicate_names.push_back(p.get_name());
    }
    create_rules(annotation_generator);

    useful_atoms.resize(task.get_initial_state().get_relations().size());

}

void Datalog::get_nullary_atoms_from_vector(const vector<bool> &nullary_predicates_in_precond,
                                            vector<size_t> &nullary_preconds) const {
    for (size_t i = 0; i < nullary_predicates_in_precond.size(); ++i) {
        if (nullary_predicates_in_precond[i]) {
            nullary_preconds.push_back(i);
        }
    }
}

void Datalog::create_rules(AnnotationGenerator ann) {
    for (const ActionSchema &schema : task.get_action_schemas()) {
        const std::vector<bool> &nullary_predicates_in_precond = schema.get_positive_nullary_precond();
        std::vector<size_t> nullary_preconds;
        get_nullary_atoms_from_vector(nullary_predicates_in_precond, nullary_preconds);
        generate_action_rule(schema, nullary_preconds, ann);
        generate_action_effect_rules(schema, ann);
        //generate_rules_with_n_ary_heads(schema, nullary_preconds);
        //generate_rules_with_nullary_heads(schema, nullary_preconds);
    }
}

void Datalog::generate_action_rule(const ActionSchema &schema,
                                   std::vector<size_t> nullary_preconds, AnnotationGenerator &annotation_generator) {
    string action_predicate = "action-" + schema.get_name();
    int idx = get_next_auxiliary_predicate_idx();
    map_new_predicates_to_idx.emplace(action_predicate, idx);
    predicate_names.push_back(action_predicate);
    DatalogAtom eff(schema, idx);
    vector<DatalogAtom> body = get_atoms_in_rule_body(schema, nullary_preconds);
    // We reverse the body because this apparently has an affect in performance for some domains
    // (e.g., logistics). This was already done in the previous implementation.
    std::reverse(body.begin(), body.end());
    std::unique_ptr<Annotation> ann = annotation_generator(schema.get_index(), task);
    rules.emplace_back(make_unique<GenericRule>(schema.get_cost(), eff, std::move(body), std::move(ann), schema.get_index()));
}

void Datalog::generate_action_effect_rules(const ActionSchema &schema, AnnotationGenerator &annotation_generator) {
    vector<DatalogAtom> body = get_action_effect_rule_body(schema);
    for (const Atom &eff : schema.get_effects()) {
        if (eff.is_negated())
            continue;
        DatalogAtom effect(eff);
        std::unique_ptr<Annotation> ann = annotation_generator(-1, task);
        rules.emplace_back(make_unique<GenericRule>(schema.get_cost(), eff, body, std::move(ann)));
    }
    const vector<bool> &nullary_predicates_in_eff = schema.get_positive_nullary_effects();
    vector<size_t> nullary_effects;
    get_nullary_atoms_from_vector(nullary_predicates_in_eff, nullary_effects);
    for (size_t eff_idx : nullary_effects) {
        DatalogAtom eff(Arguments(), eff_idx, false);
        std::unique_ptr<Annotation> ann = annotation_generator(-1, task);
        rules.emplace_back(make_unique<GenericRule>(schema.get_cost(), eff, body, std::move(ann), schema.get_index()));
    }
}

vector<DatalogAtom> Datalog::get_action_effect_rule_body(const ActionSchema &schema) {
    vector<DatalogAtom> body(1);
    string action_predicate = "action-" + schema.get_name();
    size_t idx = map_new_predicates_to_idx[action_predicate];
    body[0] = DatalogAtom(schema, idx);
    return body;
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

void Datalog::output_rule(const std::unique_ptr<RuleBase> &rule) const {
    DatalogAtom effect = rule->get_effect();
    output_atom(effect);
    size_t number_conditions = rule->get_conditions().size();
    if (number_conditions == 0) {
        cout << "." << endl;
    }
    else {
        cout << " :- ";
    }
    for (const auto &condition : rule->get_conditions()) {
        --number_conditions;
        output_atom(condition);
        if (number_conditions > 0) {
            cout << ", ";
        }
        else {
            cout << " [weight: " << rule->get_weight() << ", " << rule->get_type_name() << ", index:" << rule->get_index() << "]." << endl;
        }
    }
    rule->output_variable_table();
}

void Datalog::output_atom(const DatalogAtom &atom) const {
    std::cout << predicate_names[atom.get_predicate_index()];
    output_parameters(atom.get_arguments());
}

void Datalog::output_parameters(const Arguments& v) const {
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

const std::vector<Fact> &Datalog::get_facts() {
    return permanent_edb;
}

const std::vector<Fact> &Datalog::get_permanent_edb() {
    return permanent_edb;
}

int Datalog::get_instantiation_of_variable(const Fact &rule_head, int idx) const {
    /*
     * We get a fact (rule head) and a variable index from the variable source table
     * representing the variable we want to instantiate.
     */
    const Achievers &achiever = rule_head.get_achievers();
    const RuleBase &r = *rules[achiever.get_achiever_rule_index()];
    const VariableSource &variable_table = r.get_variable_source_object_by_ref();
    if (variable_table.is_variable_found_in_body(idx)) {
        /*
         * We look at the variable source table and check if this instantiation is found in the
         * body. If it is, we simply retrieve it from the table and return the argument.
         */
        std::pair<int, int> variable_entry = variable_table.get_table()[idx];
        int atom_position = variable_table.get_position_of_atom_in_same_body_rule(variable_entry.first);
        int fact_idx = achiever.at(atom_position);
        const Fact &achiever_fact = facts[fact_idx];
        return achiever_fact.argument(variable_entry.second).get_index();
    }
    else {
        /*
         * Otherwise, we get from the variable source table through which fact we need to backchain
         * and we call this function (get_instantiation_of_variable) recursively with this atom.
         * The 'idx' value in this recursive call (second parameter) can be obtained from the variable
         * source table which, in the case of indirect access, points to the entry in the variable
         * table of the achiever that will contain the necessary variable.
         */

        //std::cout << "rule id: " << r.get_index() << std::endl << std::flush;
        std::pair<int, int> variable_entry = variable_table.get_table()[idx];
        int fact_idx = achiever.at(variable_entry.first);
        const Fact &achiever_fact = facts[fact_idx];
        return get_instantiation_of_variable(achiever_fact, variable_entry.second);
    }
}

std::vector<int> Datalog::extract_variable_instantiation_from_rule(int head) const {
    const datalog::Fact &f = get_fact_by_index(head);
    int rule_index = f.get_achiever_rule_index();
    const RuleBase &r = *rules[rule_index];
    std::vector<int> instantiation(r.get_variable_source_object_by_ref().get_table().size());
    for (size_t i = 0; i < instantiation.size(); ++i) {
        instantiation[i] = get_instantiation_of_variable(f, i);
    }
    return instantiation;
}

void Datalog::backchain_from_goal(const Fact &goal_fact, const phmap::flat_hash_set<int> &initial_facts) {

    for (auto &relation : useful_atoms) {
        relation.clear();
    }

    phmap::flat_hash_set<int> achieved_atoms;
    std::queue<int> queue;

    for (int achiever_idx : goal_fact.get_achiever_body()) {
        if (initial_facts.count(achiever_idx) == 0) {
            queue.push(achiever_idx);
        }
    }


     while (!queue.empty()) {
         int next_achiever_idx = queue.front();
         queue.pop();
         auto is_achieved_now = achieved_atoms.insert(next_achiever_idx);
         if (!is_achieved_now.second) {
             // Previously achieved and already processed. We can skip this iteration.
             continue;
         }
         if (initial_facts.count(next_achiever_idx) > 0) {
             continue;
         }
         add_useful_atom(next_achiever_idx);
         const Fact &f = get_fact_by_index(next_achiever_idx);
         int rule_idx = f.get_achiever_rule_index();
         //std::cout << " achiever rule -> ";
         //output_rule(rules[rule_idx]);
         //std::cout << std::endl << std::flush;
         const RuleBase &rule = get_rule_by_index(rule_idx);
         rule.execute(next_achiever_idx, *this);
         for (int achiever: f.get_achiever_body()) {
             const Fact &achiever_fact = get_fact_by_index(achiever);
             //std::cout << " achiever -> ";
             //output_atom(achiever_fact);
             //std::cout << std::endl << std::flush;
             if (initial_facts.count(achiever)==0) {
                 queue.push(achiever);
             } else {
                 if (achiever_fact.get_cost() > 0) {
                     // If a fact in the EDB has cost > 0, it means it is a fact
                     // achieved by a rule with an empty body.
                     // TODO Problematic with zero-cost domains
                     queue.push(achiever);
                 }
             }
         }
     }
}

void Datalog::add_useful_atom(int achiever_idx) {
    const Fact &f = facts[achiever_idx];
    if (f.is_pred_symbol_new()) return;
    GroundAtom instantiation;
    for (const Term &t : f.get_arguments()) {
        instantiation.push_back(t.get_index());
    }
    useful_atoms[f.get_predicate_index()].push_back(std::move(instantiation));
}


int Datalog::get_number_of_facts() const {
    return facts.size();
}
