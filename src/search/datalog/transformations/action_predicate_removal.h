#ifndef SEARCH_DATALOG_TRANSFORMATIONS_ACTION_PREDICATE_REMOVAL_H_
#define SEARCH_DATALOG_TRANSFORMATIONS_ACTION_PREDICATE_REMOVAL_H_

#include "../datalog.h"

#include "../rules/rule_base.h"

namespace  datalog {

void Datalog::remove_action_predicates(AnnotationGenerator &annotation_generator, const Task &task) {
    std::vector<std::unique_ptr<RuleBase>> new_rules;

    // Axiom rules neither have an action predicate as their head (their head
    // is a task predicate) nor consist of a single action-predicate body
    // atom (their bodies contain only task predicates), so they are not part
    // of any action/effect rule pair. Keep them as they are. At this stage
    // the only new (auxiliary) predicate symbols are the action predicates.
    for (auto &rule : rules) {
        bool head_is_action_predicate = rule->get_effect().is_pred_symbol_new();
        bool body_is_action_predicate = rule->get_conditions().size() == 1
            and rule->get_conditions()[0].is_pred_symbol_new();
        if (!head_is_action_predicate and !body_is_action_predicate) {
            new_rules.emplace_back(std::move(rule));
        }
    }

    for (const auto &action_rule : rules) {
        if (action_rule && action_rule->get_effect().is_pred_symbol_new()) {
            // At this point, a new predicate symbol *must be* an action predicate
            int idx = action_rule->get_effect().get_predicate_index();
            GenericRule *action_rule_base = dynamic_cast<GenericRule *>(action_rule.get());
            for (auto &effect_rule : rules) {
                if (!effect_rule)
                    continue; // Axiom rule, already moved to new_rules
                if (effect_rule->get_conditions().size() != 1)
                    continue; // Not effect rule
                if (effect_rule->get_conditions()[0].get_predicate_index() == idx) {
                    std::unique_ptr<Annotation> ann = annotation_generator(action_rule_base->get_schema_index(), task);
                    std::unique_ptr<RuleBase> new_rule = std::make_unique<GenericRule>(action_rule->get_weight(),
                                                                                       effect_rule->get_effect(),
                                                                                       action_rule->get_conditions(),
                                                                                       std::move(ann),
                                                                                       action_rule_base->get_schema_index());
                    new_rule->update_variable_source_table(action_rule->get_variable_source_object());
                    new_rules.emplace_back(std::move(new_rule));
                }
            }
        }
    }

    rules = std::move(new_rules);
}
}

#endif //SEARCH_DATALOG_TRANSFORMATIONS_ACTION_PREDICATE_REMOVAL_H_
