#ifndef GROUNDER_RULES_PRODUCT_H
#define GROUNDER_RULES_PRODUCT_H

#include "rule_base.h"

namespace lifted_heuristic {

class ReachedFacts {
    // We do not use a vector of Facts because the Fact class is more complex than
    // what we need here for this use case.
    std::vector<Arguments> facts;

public:
    ReachedFacts() = default;

    void push_back(const Arguments &args) {
        facts.push_back(args);
    }

    bool empty() const {
        return facts.empty();
    }

    std::vector<Arguments>::const_iterator begin() const {
        return facts.begin();
    }

    std::vector<Arguments>::const_iterator end() const {
        return facts.end();
    }

};

class ProductRule : public RuleBase {
    std::vector<ReachedFacts> reached_facts_per_condition;
public:
    ProductRule(Atom eff, std::vector<Atom> c)
        : RuleBase(std::move(eff), std::move(c)),
          reached_facts_per_condition(conditions.size()) {
    }

    virtual int get_type() const override {
        return PRODUCT;
    }

    void add_reached_fact_to_condition(const Arguments &args, int position) {
        reached_facts_per_condition[position].push_back(args);
    }

    ReachedFacts &get_reached_facts_of_condition(int i) {
        return reached_facts_per_condition[i];
    }

    const std::vector<ReachedFacts> &get_reached_facts_all_conditions() const {
        return reached_facts_per_condition;
    }
};

}

#endif //GROUNDER_RULES_PRODUCT_H