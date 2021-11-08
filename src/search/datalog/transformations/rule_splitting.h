#ifndef SEARCH_DATALOG_TRANSFORMATIONS_RULE_SPLITTING_H_
#define SEARCH_DATALOG_TRANSFORMATIONS_RULE_SPLITTING_H_

#include "../datalog.h"

#include "../rules/rule_base.h"
#include "../rules/generic_rule.h"
#include "../rules/product.h"
#include "../rules/project.h"

/*
 * Implement rule-splitting by Helmert (AIJ 2009).
 */

namespace  datalog {

std::unique_ptr<RuleBase> convert_into_project_rule(const std::unique_ptr<RuleBase> &rule, AnnotationGenerator &annotation_generator, int schema_id, const Task &task) {
    std::unique_ptr<Annotation> ann = annotation_generator(schema_id, task);
    std::unique_ptr<RuleBase> project_rule = std::make_unique<ProjectRule>(rule->get_weight(), rule->get_effect(), rule->get_conditions(),  std::move(ann));
    return project_rule;
}

// Split a single rule
void split_rules(std::vector<std::unique_ptr<RuleBase>> &rules, AnnotationGenerator &annotation_generator, const Task &task) {

    // Store action schemas of all generic rules first
    std::vector<int> schemas;
    for (const auto &rule : rules) {
        GenericRule *action_rule_base = dynamic_cast<GenericRule *>(rule.get());
        schemas.push_back(action_rule_base->get_schema_index());
    }

    size_t rule_counter = 0;
    for (auto &rule : rules) {
        if (rule->get_conditions().size() == 1) {
            rule = convert_into_project_rule(rule, annotation_generator, schemas[rule_counter], task);
        }
        rule_counter++;
    }
}

}

#endif //SEARCH_DATALOG_TRANSFORMATIONS_RULE_SPLITTING_H_
