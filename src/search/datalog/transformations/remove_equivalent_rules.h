#ifndef SEARCH_DATALOG_TRANSFORMATIONS_REMOVE_EQUIVALENT_RULES_H_
#define SEARCH_DATALOG_TRANSFORMATIONS_REMOVE_EQUIVALENT_RULES_H_

#include "../datalog.h"

#include "../rules/rule_base.h"
#include "../rules/generic_rule.h"
#include "../rules/join.h"
#include "../rules/product.h"
#include "../rules/project.h"

#include <set>

namespace datalog {

bool Datalog::remove_duplicate_rules() {
    std::vector<size_t> to_be_deleted;
    std::vector<bool> will_be_deleted(rules.size(), false);
    for (size_t i = 0; i < rules.size(); i++) {
        std::set<size_t> equivalent_to_i;
        // We only consider rules that have newly introduced predicate symbols as candidates for deletion
        if (!rules[i]->get_effect().is_pred_symbol_new() or will_be_deleted[i])
            continue;

        for (size_t j = i + 1; j < rules.size(); j++) {
            if (!rules[j]->get_effect().is_pred_symbol_new() or will_be_deleted[j])
                continue;
            if (rules[i]->is_equivalent(*rules[j])) {
                equivalent_to_i.insert(rules[j]->get_effect().get_predicate_index());
                to_be_deleted.emplace_back(j);
                will_be_deleted[j] = true;
            }
        }
        DatalogAtom head_rule_i = rules[i]->get_effect();
        for (size_t j = 0; j < rules.size(); j++) {
            size_t counter = 0;
            for (const DatalogAtom &condition : rules[j]->get_conditions()) {
                if (equivalent_to_i.count(condition.get_predicate_index()) > 0) {
                    /*
                     * Whenever equivalent atoms are replaced in rule bodies, we need to use the
                     * old variable names.
                     */
                    std::vector<Term> old_args;
                    for (auto t : rules[j]->get_condition_arguments(counter)) {
                        old_args.emplace_back(t);
                    }
                    DatalogAtom new_atom = head_rule_i;
                    new_atom.update_arguments(old_args);
                    rules[j]->replace_single_condition(counter, new_atom);
                }
                ++counter;
            }
        }
    }

    // TODO Do in a smarter way, obviously.
    std::sort(to_be_deleted.begin(), to_be_deleted.end());
    std::reverse(to_be_deleted.begin(), to_be_deleted.end());
    for (size_t &i : to_be_deleted) {
        rules.erase(rules.begin() + i);
    }

    std::cout << "Iteration removed " << to_be_deleted.size() << " equivalent rules." << std::endl;
    return (to_be_deleted.size() > 0);

}

}


#endif //SEARCH_DATALOG_TRANSFORMATIONS_REMOVE_EQUIVALENT_RULES_H_
