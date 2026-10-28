#ifndef SEARCH_GROUNDED_GROUNDED_SEARCH_H
#define SEARCH_GROUNDED_GROUNDED_SEARCH_H

#include "ground_state_registry.h"

#include "../utils/system.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class GroundTask;
class GroundedHeuristic;
class Options;
class Task;

/**
 * @brief Applicable-operator computation and successor generation on the
 * ground task (with ground axiom evaluation).
 *
 * Each operator is watched by one of its preconditions (the one occurring
 * in the fewest operator preconditions); the operators watched by the true
 * facts of a state are the candidates, whose remaining (positive and
 * negated) preconditions are checked on the state's bitset.
 */
class GroundSuccessorGenerator {
    const GroundTask &task;
    int num_words;

    std::vector<int> pre_start, pre;
    std::vector<int> neg_start, neg;
    std::vector<int> watch_start, watch;
    //! Facts watching some operator.
    std::vector<uint64_t> watch_mask;
    // Operators watched by fact f, grouped by a second precondition:
    // groups [group_start[f], group_start[f + 1]), group i requires fact
    // group_fact[i] (if non-negative) and contains the operators
    // group_ops[group_op_start[i] .. group_op_start[i + 1]).
    std::vector<int> group_start, group_fact, group_op_start, group_ops;
    // For facts watching many groups: offset of the bitset of their groups'
    // facts in group_masks (or -1).
    std::vector<int> mask_start;
    std::vector<uint64_t> group_masks;
    void compute_watch_mask();
    std::vector<int> no_pre_ops;

    // Axioms, sorted by stratum.
    std::vector<int> stratum_start;
    std::vector<int> ax_body_start, ax_body;
    std::vector<int> ax_neg_start, ax_neg;
    std::vector<int> ax_head;
    std::vector<int> body_of_start, body_of;
    std::vector<int> ax_stratum;
    std::vector<uint64_t> derived_mask;
    std::vector<int> ax_unsatisfied;
    std::vector<int> ax_queue;
    bool has_axioms;

    std::vector<int> fired_ce;

public:
    explicit GroundSuccessorGenerator(const GroundTask &task);

    //! Only generate the operators marked in `allowed`.
    void restrict_operators(const std::vector<char> &allowed);

    //! Applicable operators (stops after max_ops of them).
    void get_applicable(const uint64_t *bits, const std::vector<int> &true_facts,
                        std::vector<int> &ops, size_t max_ops = SIZE_MAX) const;

    bool is_applicable(const uint64_t *bits, int op) const;

    /**
     * Compute the successor of `parent` by `op` into `child`; the facts
     * true in the child but not in the parent are appended to `added`, the
     * facts true in the parent but not in the child to `removed`.
     */
    void apply(const uint64_t *parent, int op, uint64_t *child, std::vector<int> &added,
               std::vector<int> &removed);

    void evaluate_axioms(uint64_t *bits);
};

//! Configuration of one search run of the ground search.
struct GroundSearchConfig {
    std::string name;
    bool novelty = true;
    bool ff = true;
    bool add = false;
    bool lm = false;
    int width = 1;
    //! Generate successors in random order (seeded).
    bool shuffle = false;
    unsigned seed = 0;
    //! If positive, successors with novelty greater than this are pruned
    //! (the search is incomplete). Requires the novelty open list.
    int prune_novelty = 0;
    //! If non-negative, the run stops after this many expansions. Runs with
    //! pruning or an expansion limit run first, one after the other, before
    //! the remaining runs are interleaved.
    long long max_expansions = -1;
    //! Time limit (seconds) of a staged run.
    double max_seconds = 1e9;
    //! Partition novelty by the landmark count instead of (#g, #r).
    bool novelty_by_lm = false;
    //! Compute the novelty of a state when it is expanded; its successors
    //! inherit it (as in lazy search).
    bool lazy_novelty = false;
    //! Add a lazy novelty open list to the eager one.
    bool lazy_queue = false;
    //! In a portfolio, the run starts after this many seconds.
    double start_time = 0;
};

class GroundSearchRun;
class LandmarkCountHeuristic;

/**
 * @brief Search on the ground task.
 *
 * Each search run is a greedy search alternating (round robin) between up
 * to four open lists, each with a preferred variant (successors achieving an
 * atom of the parent's relaxed plan or an interesting landmark) that gets
 * priority after every heuristic improvement:
 *  - novelty (width 1 or 2, partitioned by the number of unsatisfied goals
 *    and of unsatisfied relevant atoms), ties broken by unsatisfied goals;
 *  - h-FF of the parent (lazy evaluation);
 *  - h-add of the parent;
 *  - the landmark-count heuristic of the state.
 * Novelty plus h-FF is alternated BFWS (alt-bfws1); h-FF plus landmarks is
 * LAMA-like.
 *
 * Several runs (a portfolio) are interleaved: the run that has used the
 * least time so far expands the next batch of states. The runs share the
 * ground task, the successor generator, and the heuristics. If the runs use
 * too much memory, the largest one is dropped.
 */
class GroundedSearch {
    friend class GroundSearchRun;

    const Task &task;
    std::shared_ptr<GroundTask> ground_task;
    bool full_novelty_check;
    bool compress_chains;

    GroundSuccessorGenerator generator;
    std::unique_ptr<GroundedHeuristic> heuristic;
    std::unique_ptr<LandmarkCountHeuristic> landmarks;

    int num_words;
    std::vector<int> goal_pos;
    std::vector<int> goal_neg;
    // Relevant atoms (atoms of the relaxed plan of the initial state); the
    // second list contains the base facts of relevant complement facts.
    std::vector<int> relevant_pos;
    std::vector<int> relevant_neg;
    std::vector<uint64_t> initial_bits;
    int initial_h = 0;

    std::vector<std::unique_ptr<GroundSearchRun>> runs;
    long long staged_expanded = 0;
    long long staged_evaluated = 0;
    long long staged_generated = 0;

    int count_unsatisfied_goals(const uint64_t *bits) const;
    int count_unsatisfied_relevant(const uint64_t *bits) const;
    void print_plan_of_ops(const std::vector<int> &ops) const;

public:
    GroundedSearch(const Task &task, std::shared_ptr<GroundTask> ground_task,
                   const std::string &heuristic_name, bool full_novelty_check);
    ~GroundedSearch();

    utils::ExitCode search(const std::vector<GroundSearchConfig> &configs);
    void print_statistics() const;
};

#endif  // SEARCH_GROUNDED_GROUNDED_SEARCH_H
