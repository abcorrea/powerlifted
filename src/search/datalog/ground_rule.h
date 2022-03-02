#ifndef SEARCH_DATALOG_GROUND_RULE_H_
#define SEARCH_DATALOG_GROUND_RULE_H_

#include "datalog_fact.h"

#include "rules/rule_base.h"

namespace datalog {

class GroundRule {
    int head;
    std::vector<Fact> &facts;
public:
    GroundRule(int head, std::vector<Fact> &facts) : head(head), facts(facts) {}

    const Fact &get_head() {
        return facts[head];
    }

    const Achievers &get_achievers() {
        return get_head().get_achievers();
    }

    std::vector<int> get_parameters() {
        std::vector<int> parameters;
        /*RuleBase rule = get_head().get_achiever().get_rule();
        for (int i = 0; i < task.get_action_schema_by_index(action_schema_id).get_parameters().size(); ++i) {
            parameters.push_back(rule.get_parameter(i));
        }*/
        return parameters;
    }
};

}

#endif //SEARCH_DATALOG_GROUND_RULE_H_
