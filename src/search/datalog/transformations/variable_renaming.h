#ifndef SEARCH_DATALOG_TRANSFORMATIONS_VARIABLE_RENAMING_H_
#define SEARCH_DATALOG_TRANSFORMATIONS_VARIABLE_RENAMING_H_

#include "../datalog.h"

namespace datalog {

/*
 * NOTE: This function has the potential of breaking the variable source tables, so only use them
 * once the set of rules will not have their respective tables changed anymore. The reason why
 * this is the case is because the terms are changed and hence we cannot rely anymore on the
 * information in the auxiliary data structures of the variable source tables.
 */


void Datalog::rename_variables() {

    for (auto &rule : rules) {
        /*
         * Idea: map every variable to a constant, based on the order they appear in the rule.
         * Then replace arguments accordingly.
         */
        int _next_idx_counter = 0;
        std::unordered_set<int> variables;
        std::unordered_map<int, int> map_variable_to_new_index;

        int counter = 0;
        for (auto &c : rule->get_conditions()) {
            // We extract only variables from the conditions as we only work with Datalog programs
            // where variables in the head must occur in the body.
            std::vector<Term> new_terms;
            for (const Term &t : c.get_arguments()) {
                if (t.is_object()) {
                    new_terms.emplace_back(t);
                    continue;
                }
                int index = t.get_index();
                if (variables.find(index) == variables.end()) {
                    variables.insert(index);
                    map_variable_to_new_index.insert(std::make_pair(index, _next_idx_counter++));
                }
                new_terms.emplace_back(map_variable_to_new_index[index],
                                       VARIABLE);
            }
            rule->update_condition_arguments(counter, new_terms);
            counter++;
        }

        // Update head
        std::vector<Term> new_terms;
        const DatalogAtom &head = rule->get_effect();
        for (const Term &t : head.get_arguments()) {
            if (t.is_object()) {
                new_terms.emplace_back(t);
                continue;
            }
            int index = t.get_index();
            assert(variables.find(index) != variables.end());
            new_terms.emplace_back(map_variable_to_new_index[index],
                                       VARIABLE);
        }
        rule->update_effect_arguments(new_terms);
        rule->recreate_map_variable_position(rule->get_effect());
    }
}

}

#endif //SEARCH_DATALOG_TRANSFORMATIONS_VARIABLE_RENAMING_H_
