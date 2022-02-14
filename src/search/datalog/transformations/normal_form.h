#ifndef SEARCH_DATALOG_TRANSFORMATIONS_NORMAL_FORM_H_
#define SEARCH_DATALOG_TRANSFORMATIONS_NORMAL_FORM_H_

#include "connected_components.h"
#include "greedy_join.h"


#include "../datalog.h"

#include "../rules/rule_base.h"
#include "../rules/generic_rule.h"
#include "../rules/join.h"
#include "../rules/product.h"
#include "../rules/project.h"

#include <limits>
#include <queue>

/*
 * Implement rule-splitting by Helmert (AIJ 2009).
 */

namespace  datalog {

std::unique_ptr<RuleBase> Datalog::convert_into_project_rule(const std::unique_ptr<RuleBase> &rule,
                                                    const Task &task) {
    VariableSource old_source = rule->get_variable_source_object();
    std::unique_ptr<RuleBase> project_rule = std::make_unique<ProjectRule>(rule->get_weight(), rule->get_effect(), rule->get_conditions(),  std::move(rule->get_annotation()));
    project_rule->update_variable_source_table(std::move(old_source));
    return project_rule;
}

std::unique_ptr<RuleBase> Datalog::convert_into_product_rule(const std::unique_ptr<RuleBase> &rule,
                                                    const Task &task) {
    VariableSource old_source = rule->get_variable_source_object();
    std::unique_ptr<RuleBase> product_rule = std::make_unique<ProductRule>(rule->get_weight(),
                                                                           rule->get_effect(),
                                                                           rule->get_conditions(),
                                                                           std::move(rule->get_annotation()));
    product_rule->update_variable_source_table(std::move(old_source));
    return product_rule;
}


void Datalog::split_rule(std::vector<std::unique_ptr<RuleBase>> &join_rules, std::unique_ptr<RuleBase> &rule, std::vector<size_t> body_ids) {

    std::string predicate_name = "p$" + std::to_string(predicate_names.size());
    int idx = get_next_auxiliary_predicate_idx();
    map_new_predicates_to_idx.emplace(predicate_name, idx);
    predicate_names.push_back(predicate_name);

    std::vector<DatalogAtom> original_conditions = rule->get_conditions();
    std::vector<DatalogAtom> new_rule_conditions;
    new_rule_conditions.reserve(body_ids.size());
    for (size_t id : body_ids) {
        new_rule_conditions.push_back(original_conditions[id]);
    }

    Arguments new_args = get_conditions_arguments(new_rule_conditions);

    DatalogAtom new_atom(new_args, idx, true);
    std::unique_ptr<JoinRule> new_split_rule = std::make_unique<JoinRule>(0,
                                                                          new_atom,
                                                                          new_rule_conditions,
                                                                          nullptr);


    rule->update_conditions(new_atom,
                            new_rule_conditions,
                            new_split_rule->get_variable_source_object(),
                            std::move(body_ids));

    join_rules.push_back(std::move(new_split_rule));
}


void Datalog::convert_into_join_rules(std::vector<std::unique_ptr<RuleBase>> &join_rules,
                                      std::unique_ptr<RuleBase> &rule,
                                      const Task &task) {
    JoinCost join_cost;
    //size_t idx1 = std::numeric_limits<size_t>::max();
    //size_t idx2 = std::numeric_limits<size_t>::max();
    while(rule->get_conditions().size() > 2) {
        /*for (size_t i = 0; i < rule->get_conditions().size() - 1; ++i) {
            for (size_t j = i+1; j < rule->get_conditions().size(); ++j) {
                JoinCost cost = compute_join_cost(rule, rule->get_conditions()[i], rule->get_conditions()[j]);
                if (cost < join_cost) {
                    join_cost = cost;
                    idx1 = i;
                    idx2 = j;
                }
                std::vector<size_t> body_atoms_to_split = {idx1, idx2};
                //split_rule(rule, body_atoms_to_split);
            }
        }*/
        std::vector<size_t> indices = {0,1};
        std::sort(indices.begin(), indices.end());
        split_rule(join_rules, rule, indices);
    }
    std::unique_ptr<RuleBase> join_rule = std::make_unique<JoinRule>(rule->get_weight(),
                                                                     rule->get_effect(),
                                                                     rule->get_conditions(),
                                                                     std::move(rule->get_annotation()));

    // TODO This is wrong. Try to create method similar as the one used in connected_components.h
    join_rule->update_variable_source_table(rule->get_variable_source_object());

    join_rules.emplace_back(std::move(join_rule));
}

bool Datalog::is_product_rule(const std::unique_ptr<RuleBase> &rule) {
    std::set<int> vars;
    for (const auto &condition : rule->get_conditions()) {
        for (const auto &arg : condition.get_arguments()) {
            if (!arg.is_object()) {
                if (vars.find(arg.get_index()) != vars.end()) return false;
                vars.insert(arg.get_index());
            }
        }
    }
    return true;
}

void Datalog::convert_rules_to_normal_form(const Task &task) {
    std::vector<std::unique_ptr<RuleBase>> new_rules;

    /*
     * First step, split rules into connected components.
     */

    for (auto &rule : rules) {
        if (rule->get_conditions().size() > 1) {
            split_into_connected_components(rule, new_rules);
        }
    }
    for (auto &rule : new_rules) {
        rules.emplace_back(std::move(rule));
    }

    std::cout << "@@@@ AFTER CONNECTED COMPONENT SPLIT: " << std::endl;
    output_rules();
    std::cout << std::endl;

    new_rules.clear();

    /*** TODO This is a quick solution to the problem with constant in join rules.
        Ideally we want to either check before the join if the constants match, or make the
        rule matcher also take into account the constants.
   */
    for (auto &rule : rules) {
        if (rule->get_conditions().size() > 1) {
            /*
             * Idea: Iterate over body of rule. If there's one atom with a constant, we call a function
             * that creates a new auxiliary atom projecting out this constant and also a rule with
             * the original atom in the body and the projected atom in the head.
             *
             * Then, we replace the old body atom in the original rule by the new one. We loop over
             * the VariableSource table, looking for entries that point to the original atom. If then
             * simply need to change the indices of the second element of the variable source table.
             */

            for (size_t i = 0; i < rule->get_conditions().size(); ++i) {
                auto &condition = rule->get_conditions()[i];
                bool project_away = false;
                std::vector<Term> remaining_args;
                for (size_t j = 0; j < condition.get_arguments().size() and !project_away; ++j) {
                    if (condition.argument(j).is_object()) {
                        project_away = true;
                    } else {
                        remaining_args.push_back(condition.argument(j));
                    }
                }
                if (project_away) {
                    std::string predicate_name = "p$" + std::to_string(predicate_names.size());
                    int idx = get_next_auxiliary_predicate_idx();
                    map_new_predicates_to_idx.emplace(predicate_name, idx);
                    predicate_names.push_back(predicate_name);

                    Arguments new_args(std::move(remaining_args));
                    DatalogAtom new_atom(new_args, idx, true);
                    std::unique_ptr<RuleBase> new_rule = std::make_unique<ProjectRule>(0,
                                                                                new_atom,
                                                                                std::vector<DatalogAtom>{condition},
                                                                                nullptr);
                    rule->update_single_condition(i, new_atom);
                    new_rules.emplace_back(std::move(new_rule));
                }
            }

            std::vector<std::unique_ptr<RuleBase>> projection_rules;
        }
    }

    size_t rule_counter = 0;
    for (auto &rule : rules) {
        if (rule->get_conditions().size() == 1) {
            new_rules.push_back(convert_into_project_rule(rule, task));
        }
        else {
            if (is_product_rule(rule)) {
                new_rules.push_back(convert_into_product_rule(rule, task));
            }
            else {
                std::vector<std::unique_ptr<RuleBase>> join_rules;
                convert_into_join_rules(join_rules, rule, task);
                for (auto &join_rule : join_rules) {
                    new_rules.push_back(std::move(join_rule));
                }
            }
        }
        rule_counter++;
    }

    rules = std::move(new_rules);
}

Arguments Datalog::get_conditions_arguments(const std::vector<DatalogAtom> &conditions) {
    std::vector<Term> terms;

    for (const auto &c : conditions) {
        for (const auto &t : c.get_arguments()) {
            if (!t.is_object() and std::find(terms.begin(), terms.end(), t) == terms.end()) {
                terms.emplace_back(t);
            }
        }
    }

    return Arguments(std::move(terms));
}

Arguments Datalog::get_relevant_joining_arguments(const DatalogAtom &rule_head, const std::vector<DatalogAtom> &conditions) {
    Arguments rule_head_args = rule_head.get_arguments();
    std::vector<Term> terms;

    for (const auto &c : conditions) {
        for (const auto &t : c.get_arguments()) {
            if (!t.is_object() and (std::find(terms.begin(), terms.end(), t) == terms.end())
                and (std::find(rule_head_args.begin(), rule_head_args.end(), t) != rule_head_args.end())) {
                terms.emplace_back(t);
            }
        }
    }

    return Arguments(std::move(terms));
}


}

#endif //SEARCH_DATALOG_TRANSFORMATIONS_NORMAL_FORM_H_
