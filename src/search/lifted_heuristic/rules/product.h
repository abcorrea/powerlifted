#ifndef GROUNDER_RULES_PRODUCT_H
#define GROUNDER_RULES_PRODUCT_H

#include "rule_base.h"

namespace lifted_heuristic {

struct ProductDequeEntry {
    ProductDequeEntry(Arguments arguments, int i, int c)
    : arguments(arguments), index(i), cost(c) {}

    Arguments arguments;
    int index;
    int cost;
};

class ReachedFacts {
    // We do not use a vector of Facts because the Fact class is more complex than
    // what we need here for this use case.
    std::vector<Arguments> facts;
    std::vector<int> costs;

public:
    ReachedFacts() = default;

    void push_back(const Arguments &args, int i) {
        facts.push_back(args);
        costs.push_back(i);
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

    int get_cost(int i) const {
        return costs[i];
    }

    std::vector<int> get_costs() const {
        return costs;
    }

};

class ProductRule : public RuleBase {
    std::vector<ReachedFacts> reached_facts_per_condition;
public:
    ProductRule(int weight, Atom eff, std::vector<Atom> c)
        : RuleBase(weight, std::move(eff), std::move(c)),
          reached_facts_per_condition(conditions.size()) {
    }

    int get_type() const override {
        return PRODUCT;
    }

    void clean_up() override  {
        reached_facts_per_condition.clear();
        reached_facts_per_condition.resize(conditions.size());
    }

    void add_reached_fact_to_condition(const Arguments &args, int position, int cost) {
        reached_facts_per_condition[position].push_back(args, cost);
    }

    ReachedFacts &get_reached_facts_of_condition(int i) {
        return reached_facts_per_condition[i];
    }

    const std::vector<ReachedFacts> &get_reached_facts_all_conditions() const {
        return reached_facts_per_condition;
    }

    int get_cost_reached_fact_in_position(int position_counter, int reached_fact_index) const {
        return reached_facts_per_condition[position_counter].get_cost(reached_fact_index);
    }

};

}

#endif //GROUNDER_RULES_PRODUCT_H