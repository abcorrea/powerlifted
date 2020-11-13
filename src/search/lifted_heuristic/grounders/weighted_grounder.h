#ifndef GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_
#define GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_

#include "grounder.h"

#include "../rule_matcher.h"

#include "../../algorithms/priority_queues.h"

#include <iostream>
#include <optional>
#include <unordered_set>
#include <vector>

namespace lifted_heuristic {

const int HAS_CHEAPER_PATH = -2;

enum {H_ADD, H_MAX};

class WeightedGrounder : public Grounder {
    int is_cheapest_path_to_achieve_fact(Fact &new_fact,
                                         std::unordered_set<Fact> &reached_facts,
                                         LogicProgram &lp);

    priority_queues::AdaptiveQueue<int> q;


protected:
    int heuristic_type;

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

    std::optional<Fact> project(const RuleBase &rule, const Fact &fact);
    std::vector<Fact> join(RuleBase &rule, const Fact &fact, int position);
    std::vector<Fact> product(RuleBase &rule,
                              const Fact &fact,
                              int position);

    int aggregation_function(int i, int j)  {
        if (heuristic_type == H_ADD)
            return i + j;
        else
            return std::max(i,j);
    }

public:
    WeightedGrounder() {};

    WeightedGrounder(const LogicProgram &lp, int h)  {
        create_rule_matcher(lp);
        heuristic_type = h;
    }

    ~WeightedGrounder() = default;

    int ground(LogicProgram &lp, int goal_predicate) override;

};

}

#endif //GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_