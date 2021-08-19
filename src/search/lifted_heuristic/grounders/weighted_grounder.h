#ifndef GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_
#define GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_

#include "grounder.h"

#include "../rule_matcher.h"

#include "../../algorithms/priority_queues.h"
#include "../fact.h"

#include <iostream>
#include <unordered_set>
#include <vector>
#include <queue>

namespace lifted_heuristic {

class RuleBase;

const int HAS_CHEAPER_PATH = -2;

enum {H_ADD, H_MAX, FF, RFF};


struct RelaxedGroundAction {
    int schema;
    int cost;
    std::vector<int> precondition;

    RelaxedGroundAction(int s, int c, std::vector<int> &p)
        : schema(s), cost(c), precondition(std::move(p)) {
    }

    bool operator==(const RelaxedGroundAction &other) const {
        return (schema == other.schema) and
            (cost == other.cost) and
            (precondition == other.precondition);
    }

    bool operator!= (const RelaxedGroundAction &other) const {
        return !(*this == other);
    }
};

class WeightedGrounder : public Grounder {
    static int is_cheapest_path_to_achieve_fact(Fact &new_fact,
                                         std::unordered_set<Fact> &reached_facts,
                                         LogicProgram &lp);

    priority_queues::AdaptiveQueue<int> q;

    std::unordered_set<int> initial_facts;
    std::vector<int> useful_atoms;

    int ff_value;


    RelaxedGroundAction get_action_from_rule_achievers(const LogicProgram &lp,
                                                       std::queue<int> &achievers_queue,
                                                       const Fact &f);

    std::vector<int> reorder_rule_body(const std::vector<int> &original_permutation,
                                       const std::vector<int> &unsplit_body) const;

    void unsplit_rule(const LogicProgram &lp,
                      std::queue<int> &achievers_queue,
                      const Fact &f,
                      std::vector<int> &unsplit_body);
    int get_ff_plan_cost(const std::unordered_set<RelaxedGroundAction> &ff_plan) const;

    void collect_achievers(const LogicProgram &lp,
                           std::queue<int> &achievers_queue,
                           const Fact &fact);

    void initialize_achievers_queue(const Fact &goal_fact, std::queue<int> &achievers_queue);

protected:
    int heuristic_type;

    RuleMatcher rule_matcher;

    void create_rule_matcher(const LogicProgram &lp);

    void project(const RuleBase &rule, const Fact &fact, std::vector<Fact>& newfacts);
    void join(RuleBase &rule, const Fact &fact, int position, std::vector<Fact>& newfacts);
    void product(RuleBase &rule, const Fact &fact, int position, std::vector<Fact>& newfacts);

    int aggregation_function(int i, int j) const {
        return (heuristic_type == H_MAX) ? std::max(i, j) : i + j;
    }

public:
    WeightedGrounder(const LogicProgram &lp, int h)  {
        create_rule_matcher(lp);
        heuristic_type = h;
    }

    ~WeightedGrounder() override = default;

    int ground(LogicProgram &lp, int goal_predicate) override;

    void compute_best_achievers(const Fact &goal_fact, const LogicProgram &lp);

    const std::vector<int> &get_best_achievers() const {
        return useful_atoms;
    }

    int get_ff_value() {
        return ff_value;
    }
    int explore_achievers_queue(const LogicProgram &lp,
                                std::unordered_set<int> &achieved_atoms,
                                std::queue<int> &achievers_queue,
                                std::unordered_set<RelaxedGroundAction> &ff_plan);
};

}

namespace std {
template <>
struct hash<lifted_heuristic::RelaxedGroundAction>
{
    std::size_t operator()(const lifted_heuristic::RelaxedGroundAction& c) const {
        std::size_t result = boost::hash_range(c.precondition.begin(), c.precondition.end());
        boost::hash_combine(result, c.schema);
        return result;
    }
};
}

#endif //GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_
