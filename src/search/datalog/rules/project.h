#ifndef GROUNDER_RULES_PROJECT_RULE_H
#define GROUNDER_RULES_PROJECT_RULE_H

#include "rule_base.h"

namespace datalog {

class ProjectRule : public RuleBase {
public:
    using RuleBase::RuleBase;

    void clean_up() override {
    }
    int get_type() const override {
        return PROJECT;
    }

    const Arguments &get_condition_arguments() const {
        return conditions[0].get_arguments();
    }

    std::string get_type_name() override {
        return "ProjectRule";
    }
};

}

#endif //GROUNDER_RULES_PROJECT_RULE_H
