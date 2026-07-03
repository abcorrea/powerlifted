#ifndef SEARCH_DATALOG_RULES_GENERIC_RULE_H_
#define SEARCH_DATALOG_RULES_GENERIC_RULE_H_

#include "rule_base.h"

namespace datalog {

class GenericRule : public RuleBase {

    // Rules created through the inherited constructor have no associated
    // action schema; -1 makes the annotation generators return no annotation.
    int schema_index = -1;

public:
    using RuleBase::RuleBase;

    GenericRule(int weight,
        DatalogAtom eff,
        std::vector<DatalogAtom> c,
        std::unique_ptr<Annotation> annotation,
        int schema_index) : RuleBase(weight, eff, c, std::move(annotation)), schema_index(schema_index) {

    }

    void clean_up() override {}

    int get_type() const override {
        return GENERIC;
    }

    int get_schema_index() const {
        return schema_index;
    }

    std::string get_type_name() override {
        return "GenericRule";
    }
};
}


#endif //SEARCH_DATALOG_RULES_GENERIC_RULE_H_
