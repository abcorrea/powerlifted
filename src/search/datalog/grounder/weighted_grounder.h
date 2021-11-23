#ifndef GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_
#define GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_

#include "grounder.h"

#include "../achievers.h"
#include "../datalog_fact.h"
#include "../rule_matcher.h"

#include "../../algorithms/priority_queues.h"

#include <iostream>
#include <unordered_set>
#include <vector>

namespace datalog {

class RuleBase;

const int HAS_CHEAPER_PATH = -2;

enum {H_ADD, H_MAX};

class WeightedGrounder : public Grounder {
    static int is_cheapest_path_to_achieve_fact(Fact &new_fact,
                                         std::unordered_set<Fact> &reached_facts,
                                         Datalog &lp);

    priority_queues::AdaptiveQueue<int> q;

    std::unordered_set<int> initial_facts;
    std::vector<int> best_achievers;

protected:
    int heuristic_type;

    RuleMatcher rule_matcher;

    void create_rule_matcher(const Datalog &lp);

    void project(const RuleBase &rule, const Fact &fact, std::vector<Fact>& newfacts);
    void join(RuleBase &rule, const Fact &fact, int position, std::vector<Fact>& newfacts);
    void product(RuleBase &rule, const Fact &fact, int position, std::vector<Fact>& newfacts);

    int aggregation_function(int i, int j) const {
        return (heuristic_type == H_ADD) ? i + j : std::max(i, j);
    }

public:
    WeightedGrounder(const Datalog &lp, int h)  {
        create_rule_matcher(lp);
        heuristic_type = h;
    }

    ~WeightedGrounder() override = default;

    int ground(Datalog &datalog, std::vector<Fact> &state_facts, int goal_predicate) override;

    void compute_best_achievers(const Fact &fact, const Datalog &lp);

    const std::vector<int> &get_best_achiever_indices() const {
        return best_achievers;
    }


};

}

#endif //GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_
