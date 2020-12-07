#ifndef GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_
#define GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_

#include "grounder.h"

#include "../rule_matcher.h"

#include "../../algorithms/priority_queues.h"
#include "../fact.h"

#include <iostream>
#include <unordered_set>
#include <vector>

namespace lifted_heuristic {

class RuleBase;

const int HAS_CHEAPER_PATH = -2;

enum {H_ADD, H_MAX};

class WeightedGrounder : public Grounder {
    static int is_cheapest_path_to_achieve_fact(Fact &new_fact,
                                         std::unordered_set<Fact> &reached_facts,
                                         LogicProgram &lp);

    priority_queues::AdaptiveQueue<int> q;

    std::unordered_set<int> facts_in_edb;
    Achievers best_achievers;

protected:
    int heuristic_type;

    RuleMatcher rule_matcher;

    void create_rule_matcher(const LogicProgram &lp);

    std::vector<Fact> project(const RuleBase &rule, const Fact &fact);
    std::vector<Fact> join(RuleBase &rule, const Fact &fact, int position);
    std::vector<Fact> product(RuleBase &rule,
                              const Fact &fact,
                              int position);

    int aggregation_function(int i, int j) const {
        return (heuristic_type == H_ADD) ? i + j : std::max(i, j);
    }

public:
    WeightedGrounder(const LogicProgram &lp, int h)  {
        create_rule_matcher(lp);
        heuristic_type = h;
    }

    ~WeightedGrounder() override = default;

    int ground(LogicProgram &lp, int goal_predicate) override;

    void compute_best_achievers(const Fact &fact, const LogicProgram &lp);

    const Achievers &get_best_achievers() const {
        return best_achievers;
    }


};

}

#endif //GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_
