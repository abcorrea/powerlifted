#ifndef SEARCH_DATALOG_TRANSFORMATIONS_VARIABLE_PROJECTION_H_
#define SEARCH_DATALOG_TRANSFORMATIONS_VARIABLE_PROJECTION_H_

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

namespace datalog {

void Datalog::project_out_variables(std::unique_ptr<RuleBase> &rule,
                                    std::vector<std::unique_ptr<RuleBase>> &new_rules) {
    /*
     * Idea: loop over head collecting all variables in it (relevant variables), then loop over the
     * conditions counting how many times each variable appears.
     *
     * If a variable is not relevant and appears only in one atom, then it can be projected out
     * from this atom in a previous rule.
     */

    std::unordered_map<int, int> variable_counter;
    std::unordered_set<int> relevant_variables;

    // Collect relevant variables
    for (const auto &arg : rule->get_effect_arguments()) {
        if (arg.is_object()) continue;
        int index = arg.get_index();
        relevant_variables.emplace(index);
    }

    // Count how many times each variables appear in the body. It is important here that there is no
    // predicate where a same variable occurs multiple times. This should not be the case, as the
    // Python translator of Powerlifted deals with this redundancy before reaching this stage.
    for (size_t i = 0; i < rule->get_conditions().size(); ++i) {
        for (const auto &arg : rule->get_condition_arguments(i)) {
            if (arg.is_object()) continue;
            int index = arg.get_index();
            if (variable_counter.find(index) == variable_counter.end())
                variable_counter.emplace(std::make_pair(index, 1));
            else
                variable_counter[index]++;
        }
    }

    for (size_t i = 0; i < rule->get_conditions().size(); ++i) {
        bool project_out = false;
        std::vector<int> relevant_variables_for_condition;
        for (const auto &arg : rule->get_condition_arguments(i)) {
            if (arg.is_object()) continue;
            int index = arg.get_index();
            if ((variable_counter[index] > 1) or std::find(relevant_variables.begin(), relevant_variables.end(), index) != relevant_variables.end())
                relevant_variables_for_condition.push_back(index);
            else {
                project_out = true;
            }
        }
        if (project_out) {
            std::string predicate_name = "p$" + std::to_string(predicate_names.size());
            int idx = get_next_auxiliary_predicate_idx();
            map_new_predicates_to_idx.emplace(predicate_name, idx);
            predicate_names.push_back(predicate_name);

            Arguments new_args;
            for (auto v : relevant_variables_for_condition) {
                new_args.push_back(v, VARIABLE);
            }

            DatalogAtom new_atom(new_args, idx, true);
            std::vector<DatalogAtom> new_condition;
            new_condition.push_back(rule->get_conditions()[i]);

            std::unique_ptr<RuleBase> new_rule = std::make_unique<ProjectRule>(0,
                                                                     new_atom,
                                                                     new_condition,
                                                                     nullptr);

            // Update old rule
            rule->set_specific_condition(i, new_atom);
            VariableSource new_source = rule->get_variable_source_object();
            int counter = 0;
            for (auto entry : new_source.get_table()) {
                if (entry.first > 0 or (new_source.get_position_of_atom_in_same_body_rule(entry.first) != int(i))) {
                    counter++;
                    continue;
                }

                int term = new_source.get_term_from_table_entry_index(counter);
                int new_position_in_condition = i;
                int new_position_in_indirect_table = -1;

                auto source_new_split_rule = new_rule->get_variable_source_object_by_ref();
                int entry_position_indirect_table = source_new_split_rule.get_table_entry_index_from_term(term);
                new_position_in_indirect_table = entry_position_indirect_table ;

                new_source.update_ith_entry(counter, new_position_in_condition, new_position_in_indirect_table);
                counter++;
            }

            rule->update_variable_source_table(std::move(new_source));

            new_rules.push_back(std::move(new_rule));
        }
    }
}

}

#endif //SEARCH_DATALOG_TRANSFORMATIONS_VARIABLE_PROJECTION_H_
