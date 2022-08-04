#ifndef SEARCH_DATALOG_DATALOG_H_
#define SEARCH_DATALOG_DATALOG_H_

#include "rules/rule_base.h"

#include "../atom.h"
#include "../task.h"

#include "../parallel_hashmap/phmap.h"

#include <map>
#include <memory>
#include <vector>

namespace datalog {

class Datalog {

    std::vector<Fact> facts;
    std::vector<Fact> permanent_edb;
    std::vector<std::unique_ptr<RuleBase>> rules;

    // Is this what we want?
    const Task &task;

    int goal_atom_idx;

    std::vector<std::string> predicate_names;
    std::unordered_map<std::string, int> map_new_predicates_to_idx;

    std::vector<std::vector<GroundAtom>> useful_atoms;

    void create_rules(AnnotationGenerator ann);
    void generate_action_rule(const ActionSchema &schema, std::vector<size_t> nullary_preconds, AnnotationGenerator &annotation_generator);

    void generate_action_effect_rules(const ActionSchema &schema, AnnotationGenerator &annotation_generator);

    std::vector<DatalogAtom> get_action_effect_rule_body(const ActionSchema &schema);
    void get_nullary_atoms_from_vector(const std::vector<bool> &nullary_predicates_in_precond,
                                       std::vector<size_t> &nullary_preconds) const;
    std::vector<DatalogAtom> get_atoms_in_rule_body(const ActionSchema &schema,
                                                    const std::vector<size_t> &nullary_preconds) const;


    int get_next_auxiliary_predicate_idx() {
        return predicate_names.size();
    }

    void output_rule(const std::unique_ptr<RuleBase> &rule) const;
    void output_parameters(const Arguments& v) const;

    void get_always_reachable_rule_heads();

    std::unique_ptr<RuleBase> convert_into_project_rule(const std::unique_ptr<RuleBase> &rule,
                                                        const Task &task);
    std::unique_ptr<RuleBase> convert_into_product_rule(const std::unique_ptr<RuleBase> &rule,
                                                        const Task &task);
    void convert_into_join_rules(std::vector<std::unique_ptr<RuleBase>> &join_rules,
                                 std::unique_ptr<RuleBase> &rule,
                                 const Task &task);
    bool is_product_rule(const std::unique_ptr<RuleBase> &rule);

    void split_rule(std::vector<std::unique_ptr<RuleBase>> &join_rules,
                    std::unique_ptr<RuleBase> &rule, std::vector<size_t> body_ids);

    void split_into_connected_components(std::unique_ptr<RuleBase> &rule, std::vector<std::unique_ptr<RuleBase>> &new_rules);

    void project_out_variables(std::unique_ptr<RuleBase> &rule, std::vector<std::unique_ptr<RuleBase>> &new_rules);

    DatalogAtom split_connected_component(std::unique_ptr<RuleBase> &original_rule, const std::vector<int> &component, std::vector<std::unique_ptr<RuleBase>> &new_rules, int component_counter);

    Arguments get_conditions_arguments(const std::vector<DatalogAtom> &conditions);

    Arguments get_relevant_joining_arguments_from_component(const DatalogAtom &rule_head, const std::vector<DatalogAtom> &conditions);

    Arguments get_relevant_arguments_for_split(const std::unique_ptr<RuleBase> &original_rule,
                                               const std::vector<DatalogAtom> &conditions_new_rule,
                                               const std::vector<size_t> body_ids);


    int get_instantiation_of_variable(const Fact &rule_head, int idx) const;

    void add_useful_atom(int achiever_idx);

public:
    Datalog(const Task &task, AnnotationGenerator annotation_generator);


    std::vector<std::unique_ptr<RuleBase>> &get_rules() {
        return rules;
    }

    const std::vector<std::unique_ptr<RuleBase>> &get_rules() const {
        return rules;
    }

    void remove_action_predicates(AnnotationGenerator &annotation_generator, const Task &task);

    void convert_rules_to_normal_form(const Task &task);

    bool remove_duplicate_rules();

    void set_permanent_edb(StaticInformation static_information);

    void rename_variables();

    void output_permanent_edb();

    void add_goal_rule(const Task &task, AnnotationGenerator &annotation_generator);

    void output_rules() const {
        for (const auto &rule : rules) output_rule(rule);
    }

    int get_goal_atom_idx() {
        return goal_atom_idx;
    }

    const std::vector<Fact> &get_permanent_edb();

    const std::vector<Fact> &get_facts();

    const Fact &get_fact_by_index(int i) const {
        return facts[i];
    }

    RuleBase &get_rule_by_index(int index) {
        return *rules[index];
    }

    void insert_fact(const Fact &f) {
        facts.push_back(f);
    }

    void update_fact_cost(int fact, int cost) {
        facts[fact].set_cost(cost);
    }

    void update_rule_indices() {
        for (size_t i = 0; i < rules.size(); ++i) {
            rules[i]->update_index(int(i));
        }
    }

    void output_fact(const Fact &f) const {
        output_atom(f);
    }

    void reset_facts() {
        facts.clear();
    }

    void print_statistics() {
        std::cout << "Total number of static atoms in the EDB: " << permanent_edb.size() << std::endl;
        std::cout << "Total number of rules: " << rules.size() << std::endl;
    }

    std::vector<int> extract_variable_instantiation_from_rule(int head) const;

    const std::vector<std::vector<GroundAtom>> &get_useful_atoms() const {
        return useful_atoms;
    }

    void backchain_from_goal(const Fact &goal_fact, const phmap::flat_hash_set<int> &state_facts);

    int get_number_of_facts() const;
    void output_atom(const DatalogAtom &atom) const;
};

}

#endif //SEARCH_DATALOG_DATALOG_H_
