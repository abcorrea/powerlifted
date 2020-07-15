#ifndef GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_
#define GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_

#include "grounder.h"

#include "../rule_matcher.h"

#include <iostream>
#include <optional>
#include <unordered_set>

namespace lifted_heuristic {

class WeightedGrounder : public Grounder {
    bool is_new(Fact &new_fact,
                std::unordered_set<Fact> &reached_facts,
                LogicProgram &lp);

protected:
    RuleMatcher rule_matcher;

    void create_rule_matcher(const LogicProgram &lp) {
        // Loop over rule conditions
        for (const auto &rule : lp.get_rules()) {
            int cont = 0;
            for (const auto &condition : rule->get_conditions()) {
                rule_matcher.insert(condition.get_predicate_index(),
                                    rule->get_index(),
                                    cont++);
            }
        }
    }

    static std::optional<Fact> project(const RuleBase &rule, const Fact &fact);
    static std::vector<Fact> join(RuleBase &rule, const Fact &fact, int position);
    static std::vector<Fact> product(RuleBase &rule,
                                     const Fact &fact,
                                     int position);

public:
    WeightedGrounder() {};

    WeightedGrounder(const LogicProgram &lp) {
        create_rule_matcher(lp);
    }

    ~WeightedGrounder() = default;

    int ground(LogicProgram &lp) override;

};

}

#endif //GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_
