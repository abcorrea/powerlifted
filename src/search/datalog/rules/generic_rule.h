#ifndef SEARCH_DATALOG_RULES_GENERIC_RULE_H_
#define SEARCH_DATALOG_RULES_GENERIC_RULE_H_

#include "rule_base.h"

namespace datalog {

class GenericRule : public RuleBase {
public:
    using RuleBase::RuleBase;

    void clean_up() override {}

    int get_type() const override {
        return GENERIC;
    }
};
}


#endif //SEARCH_DATALOG_RULES_GENERIC_RULE_H_
