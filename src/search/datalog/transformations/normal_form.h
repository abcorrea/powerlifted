#ifndef SEARCH_DATALOG_TRANSFORMATIONS_NORMAL_FORM_H_
#define SEARCH_DATALOG_TRANSFORMATIONS_NORMAL_FORM_H_

#include "connected_components.h"
#include "greedy_join.h"
#include "variable_projection.h"


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

void add_missing_entries_to_source_table(int position, std::vector<std::unique_ptr<RuleBase>> &join_rules,
                                         const std::vector<DatalogAtom> &new_rule_conditions,
                                         std::unique_ptr<JoinRule> &new_split_rule,
                                         std::vector<int> &term_indices_in_new_args) {
    int idx_condition = new_rule_conditions[position].get_predicate_index();
    VariableSource source = new_split_rule->get_variable_source_object();
    for (const auto &join_rule : join_rules) {
        if (join_rule->get_effect().get_predicate_index() == idx_condition) {
            const VariableSource source_join_rule = join_rule->get_variable_source_object_by_ref();
            for (size_t entry_table_counter = 0; entry_table_counter < source_join_rule.get_table().size(); ++entry_table_counter) {
                int entry_term = source_join_rule.get_term_from_table_entry_index(entry_table_counter);
                if (std::find(term_indices_in_new_args.begin(), term_indices_in_new_args.end(), entry_term) == term_indices_in_new_args.end()) {
                    source.add_entry(entry_term, position, entry_table_counter);
                    term_indices_in_new_args.push_back(entry_term);
                }
            }
        }
    }
    new_split_rule->update_variable_source_table(std::move(source));
}

std::unique_ptr<RuleBase> Datalog::convert_into_project_rule(const std::unique_ptr<RuleBase> &rule,
                                                    const Task &task) {
    VariableSource old_source = rule->get_variable_source_object();
    std::unique_ptr<RuleBase> project_rule = std::make_unique<ProjectRule>(rule->get_weight(), rule->get_effect(), rule->get_conditions(),  rule->get_annotation());
    project_rule->update_variable_source_table(std::move(old_source));
    return project_rule;
}

std::unique_ptr<RuleBase> Datalog::convert_into_product_rule(const std::unique_ptr<RuleBase> &rule,
                                                    const Task &task) {
    VariableSource old_source = rule->get_variable_source_object();
    std::unique_ptr<RuleBase> product_rule = std::make_unique<ProductRule>(rule->get_weight(),
                                                                           rule->get_effect(),
                                                                           rule->get_conditions(),
                                                                           rule->get_annotation());
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

    //Arguments new_args = get_conditions_arguments(new_rule_conditions);
    Arguments new_args = get_relevant_arguments_for_split(rule, new_rule_conditions, body_ids);

    DatalogAtom new_atom(new_args, idx, true);
    std::unique_ptr<JoinRule> new_split_rule = std::make_unique<JoinRule>(0,
                                                                          new_atom,
                                                                          new_rule_conditions,
                                                                          nullptr);

    // We need to get the entries of the variables in the tables of the conditions that were not
    // carried to the new split rule (because these variables have been projected out).
    // This part is unfortunately very inefficient....

    std::vector<int> term_indices_in_new_args;
    for (const auto &c : new_split_rule->get_conditions()) {
        for (const Term &t: c.get_arguments()) {
            if (t.is_object()) continue;
            term_indices_in_new_args.push_back(t.get_index());
        }
    }

    add_missing_entries_to_source_table(0, join_rules,
                                        new_rule_conditions,
                                        new_split_rule,
                                        term_indices_in_new_args);
    add_missing_entries_to_source_table(1, join_rules,
                                        new_rule_conditions,
                                        new_split_rule,
                                        term_indices_in_new_args);

    rule->update_conditions(new_atom,
                            new_rule_conditions,
                            new_split_rule->get_variable_source_object(),
                            std::move(body_ids));


    join_rules.push_back(std::move(new_split_rule));
}



void Datalog::convert_into_join_rules(std::vector<std::unique_ptr<RuleBase>> &join_rules,
                                      std::unique_ptr<RuleBase> &rule,
                                      const Task &task) {
    while(rule->get_conditions().size() > 2) {
        JoinCost join_cost;
        size_t idx1 = std::numeric_limits<size_t>::max();
        size_t idx2 = std::numeric_limits<size_t>::max();
        for (size_t i = 0; i < rule->get_conditions().size() - 1; ++i) {
            for (size_t j = i+1; j < rule->get_conditions().size(); ++j) {
                JoinCost cost = compute_join_cost_fast_downward(rule,
                                                              rule->get_conditions()[i],
                                                              rule->get_conditions()[j]);
                if (cost < join_cost) {
                    join_cost = cost;
                    idx1 = i;
                    idx2 = j;
                }
            }
        }
        std::vector<size_t> indices = {idx1,idx2};
        std::sort(indices.begin(), indices.end());
        split_rule(join_rules, rule, indices);
    }
    std::unique_ptr<RuleBase> join_rule = std::make_unique<JoinRule>(rule->get_weight(),
                                                                     rule->get_effect(),
                                                                     rule->get_conditions(),
                                                                     rule->get_annotation());


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
                    rule->update_single_condition_and_variable_source_table(i, new_atom);
                    new_rules.emplace_back(std::move(new_rule));
                }
            }

            std::vector<std::unique_ptr<RuleBase>> projection_rules;
        }
    }

    /*
     * This part is commented out because Fast Downward does not seem to do it.
     *  Project out variables that are not relevant to the join. This means variables that:
     * (i) do not join with any other atom in the rule condition; AND (important *AND* and not *OR*)
     * (ii) do not appear in the head of the rule.
     */

    /*for (auto &rule : rules) {
        if (rule->get_conditions().size() <= 1) continue;
        project_out_variables(rule, new_rules);
    }
    for (auto &rule : new_rules) {
        rules.emplace_back(std::move(rule));
    }
    new_rules.clear();*/


    /*
     * Last step, transform rules into product/project rules or split them into multiple join rules.
     */
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

Arguments Datalog::get_relevant_joining_arguments_from_component(const DatalogAtom &rule_head, const std::vector<DatalogAtom> &conditions) {
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

Arguments Datalog::get_relevant_arguments_for_split(const std::unique_ptr<RuleBase> &original_rule,
                                                    const std::vector<DatalogAtom> &conditions_new_rule,
                                                    const std::vector<size_t> body_ids) {
    const DatalogAtom &rule_head = original_rule->get_effect();
    Arguments rule_head_args = rule_head.get_arguments();
    std::vector<Term> terms_in_new_conditions;

    for (const auto &c : conditions_new_rule) {
        for (const auto &t: c.get_arguments()) {
            if (!t.is_object()
                and (std::find(terms_in_new_conditions.begin(), terms_in_new_conditions.end(), t)
                    ==terms_in_new_conditions.end())) {
                terms_in_new_conditions.emplace_back(t);
            }
        }
    }

    std::vector<Term> final_terms;
    int counter = 0;
    for (const auto &c : original_rule->get_conditions()) {
        if (std::find(body_ids.begin(), body_ids.end(), counter) != body_ids.end()) {
            ++counter;
            continue;
        }
        for (const auto &t : c.get_arguments()) {
            if (!t.is_object() and (std::find(terms_in_new_conditions.begin(), terms_in_new_conditions.end(), t) != terms_in_new_conditions.end())
                and std::find(final_terms.begin(), final_terms.end(), t) == final_terms.end()) {
                // joining terms
                final_terms.emplace_back(t);
            }
        }
        ++counter;
    }
    for (const Term &t : rule_head_args) {
        if (!t.is_object() and (std::find(terms_in_new_conditions.begin(), terms_in_new_conditions.end(), t) != terms_in_new_conditions.end())
            and std::find(final_terms.begin(), final_terms.end(), t) == final_terms.end()) {
            final_terms.emplace_back(t);
        }
    }

    return Arguments(std::move(final_terms));
}


}

#endif //SEARCH_DATALOG_TRANSFORMATIONS_NORMAL_FORM_H_
