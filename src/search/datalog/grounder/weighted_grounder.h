#ifndef GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_
#define GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_

#include "grounder.h"

#include "../achievers.h"
#include "../datalog_fact.h"
#include "../rule_matcher.h"

#include "../../algorithms/priority_queues.h"
#include "../../parallel_hashmap/phmap.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <unordered_set>
#include <vector>

namespace datalog {

class RuleBase;

const int HAS_CHEAPER_PATH = -2;

enum {H_ADD, H_MAX};

class WeightedGrounder : public Grounder {
    int is_cheapest_path_to_achieve_fact(Fact &new_fact,
                                         phmap::flat_hash_set<Fact> &reached_facts,
                                         Datalog &lp);

    priority_queues::AdaptiveQueue<int> q;

    // The EDB and state facts are the first facts created each grounding, so they
    // occupy the contiguous fact-index range [0, num_initial_facts). Testing
    // "is this an initial fact" is therefore a single comparison — no need for a
    // per-evaluation hash set of their indices.
    int num_initial_facts;
    std::vector<int> best_achievers;

    // Persistent base: EDB facts whose predicate no rule derives can never
    // collide with a runtime-derived fact, so the first ground() call inserts
    // them into the fact vector once (indices [0, num_base_facts)) and they
    // never enter the reached-facts set; later calls re-push their indices
    // without re-copying or re-hashing them. EDB facts of derivable
    // predicates (kept in improvable_edb) get the classic per-call treatment,
    // because a runtime derivation may improve their cost.
    bool base_initialized;
    int num_base_facts;
    std::vector<Fact> improvable_edb;

    // Member instead of a ground() local so its capacity survives across
    // calls (state facts + derived facts only; see num_base_facts).
    phmap::flat_hash_set<Fact> reached_facts;

    // A* for lightest derivations (Felzenszwalb & McAllester): each call
    // computes exact inside/outside costs of the predicate-level abstraction
    // (one node per predicate, one hyperedge per rule). A predicate with
    // infinite outside cost can never take part in a goal derivation, so its
    // facts skip the queue under either ordering policy. Ordering the queue
    // by f = g + outside truncates the evaluation earlier; outside estimates
    // from an abstraction are admissible and monotone, so the goal still
    // pops at its exact cost, and f = g + outside holds under both
    // aggregations: chain weights add along a derivation even under h^max.
    // Heuristics that extract achievers (h^ff, h^rff) keep the plain cost
    // order instead: the f order pops equally cheap derivations in a
    // different sequence, records different (equally valid) achievers, and
    // thereby changes the extracted relaxed plan, whereas the cost order
    // reproduces the plain Dijkstra grounder's achievers exactly.
    static constexpr int ABSTRACT_INF = std::numeric_limits<int>::max() / 4;
    std::vector<int> abstract_inside;
    std::vector<int> abstract_outside;
    bool order_by_outside;

    void compute_abstract_costs(Datalog &datalog,
                                const std::vector<Fact> &state_facts,
                                int goal_predicate);

    int priority_of(const Fact &fact) const {
        int pred = fact.get_predicate_index();
        // Goal-less evaluation (static-stratum materialization) computes no
        // outside costs and runs as a plain Dijkstra either way.
        int out = pred < int(abstract_outside.size()) ? abstract_outside[pred] : 0;
        if (out >= ABSTRACT_INF) return ABSTRACT_INF;
        return order_by_outside ? fact.get_cost() + out : fact.get_cost();
    }

    // Reused across join() calls so the per-call join key is built in place
    // instead of allocating a fresh vector every time (join() is the hottest
    // path in the grounder). Same small-buffer-optimized type as JoinHashKey.
    utils::small_vector<int, 2> join_key_buffer;

    int queue_pushes;
    int atoms_produced;
    // Sums over all ground() calls of the search. Unlike the per-call
    // counters, nothing resets these, so the planner can report one total at
    // the end of the search.
    unsigned long long cumulative_atoms_produced;
    unsigned long long cumulative_queue_pushes;
    int total_number_of_facts;

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
    WeightedGrounder(const Datalog &lp, int h, bool order_by_outside)
        : order_by_outside(order_by_outside) {
        create_rule_matcher(lp);
        heuristic_type = h;
        queue_pushes = 0;
        atoms_produced = 0;
        cumulative_atoms_produced = 0;
        cumulative_queue_pushes = 0;
        total_number_of_facts = 0;
        base_initialized = false;
        num_base_facts = 0;
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

    unsigned long long get_cumulative_atoms_produced() const {
        return cumulative_atoms_produced;
    }

    unsigned long long get_cumulative_queue_pushes() const {
        return cumulative_queue_pushes;
    }


};

}

#endif //GROUNDER_GROUNDERS_FAST_DOWNWARD_GROUNDER_H_
