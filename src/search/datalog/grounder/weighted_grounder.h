#ifndef GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_
#define GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_

#include "grounder.h"

#include "../achievers.h"
#include "../datalog_fact.h"
#include "../rule_matcher.h"

#include "../../algorithms/priority_queues.h"
#include "../../parallel_hashmap/phmap.h"

#include <iostream>
#include <unordered_set>
#include <vector>

namespace datalog {

class RuleBase;

const int HAS_CHEAPER_PATH = -2;

enum {H_ADD, H_MAX};

class WeightedGrounder : public Grounder {
    // Probe reached_facts with the (achiever-free) head atom and commit it only
    // if it is new or strictly cheaper. The achiever is built lazily through
    // build_achievers() and ONLY when the fact is first reached -- the discard
    // and cheaper-path branches never read an achiever (backchaining reads
    // achievers from lp, which is written once at first insertion and never
    // updated on a cheaper path; the achiever stored in reached_facts is dead),
    // so building it there is wasted work. Defined in the .cc next to its uses.
    template <typename BuildAchievers>
    void process_new_fact(Fact &new_fact,
                          phmap::flat_hash_set<Fact> &reached_facts,
                          Datalog &lp,
                          BuildAchievers &&build_achievers);

    priority_queues::AdaptiveQueue<int> q;

    phmap::flat_hash_set<int> initial_facts;
    std::vector<int> best_achievers;

    int queue_pushes;
    int atoms_produced;
    int total_number_of_facts;

protected:
    int heuristic_type;

    RuleMatcher rule_matcher;

    void create_rule_matcher(const Datalog &lp);

    void project(const RuleBase &rule, const Fact &fact,
                 phmap::flat_hash_set<Fact> &reached_facts, Datalog &lp);
    void join(RuleBase &rule, const Fact &fact, int position,
              phmap::flat_hash_set<Fact> &reached_facts, Datalog &lp);
    void product(RuleBase &rule, const Fact &fact, int position,
                 phmap::flat_hash_set<Fact> &reached_facts, Datalog &lp);

    int aggregation_function(int i, int j) const {
        return (heuristic_type == H_ADD) ? i + j : std::max(i, j);
    }

public:
    WeightedGrounder(const Datalog &lp, int h)  {
        create_rule_matcher(lp);
        heuristic_type = h;
        queue_pushes = 0;
        atoms_produced = 0;
        total_number_of_facts = 0;
    }

    ~WeightedGrounder() override = default;

    int ground(Datalog &datalog, std::vector<Fact> &state_facts, int goal_predicate) override;

    void print_statistics(const Datalog &lp) override {
        std::cout << lp.get_number_of_facts() << " final number of facts" << std::endl;
        std::cout << atoms_produced << " total atoms produced" << std::endl;
        std::cout << queue_pushes << " total queue pushes" << std::endl;
    }

    const std::vector<int> &get_best_achiever_indices() const {
        return best_achievers;
    }


};

}

#endif //GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_
