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

    void get_applicable(const uint64_t *bits, const std::vector<int> &true_facts,
                        std::vector<int> &ops) const;

    /**
     * Compute the successor of `parent` by `op` into `child`; the facts
     * true in the child but not in the parent are appended to `added`, the
     * facts true in the parent but not in the child to `removed`.
     */
    void apply(const uint64_t *parent, int op, uint64_t *child, std::vector<int> &added,
               std::vector<int> &removed);

    void evaluate_axioms(uint64_t *bits);
};

/**
 * @brief Search on the ground task.
 *
 * Greedy search alternating (round robin) between up to three open lists,
 * each with a preferred variant (successors achieving an atom of the
 * parent's relaxed plan or an interesting landmark) that gets priority
 * after every heuristic improvement:
 *  - novelty (width 1 or 2, partitioned by the number of unsatisfied goals
 *    and of unsatisfied relevant atoms), ties broken by unsatisfied goals;
 *  - the delete-relaxation heuristic of the parent (lazy evaluation);
 *  - the landmark-count heuristic of the state.
 * Novelty plus FF is alternated BFWS (alt-bfws1); FF plus landmarks is
 * LAMA-like.
 */
class GroundedSearch {
    const Task &task;
    std::shared_ptr<GroundTask> ground_task;
    std::string heuristic_name;
    bool full_novelty_check;
    int width;
    bool use_novelty;
    bool use_ff;
    bool use_add;
    bool use_lm;
    bool compress_chains;

    GroundStateRegistry registry;
    GroundSuccessorGenerator generator;
    std::unique_ptr<GroundedHeuristic> heuristic;

    int num_words;
    std::vector<int> goal_pos;
    std::vector<int> goal_neg;

    // Search nodes, indexed by state id.
    std::vector<int> parent;
    std::vector<int> parent_op;
    std::vector<int> g_value;
    std::vector<short> num_unsat_goals;
    std::vector<int> num_unsat_relevant;
    std::vector<char> closed;
    //! Operator sequences of compressed chains; parent_op[s] = -k - 2 refers
    //! to chains[k].
    std::vector<std::vector<int>> chains;
    long long compressed_chains = 0;
    long long chain_length_sum = 0;
    long long pruned_dead_ends = 0;

    long long expanded = 0;
    long long generated = 0;
    long long evaluated = 0;

    int count_unsatisfied_goals(const uint64_t *bits) const;
    bool is_goal(const uint64_t *bits) const {
        return count_unsatisfied_goals(bits) == 0;
    }
    void extract_and_print_plan(int state_id) const;

public:
    GroundedSearch(const Task &task, std::shared_ptr<GroundTask> ground_task,
                   const std::string &heuristic_name, bool full_novelty_check, int width,
                   bool use_novelty, bool use_ff, bool use_add, bool use_lm);
    ~GroundedSearch();

    utils::ExitCode search();
    void print_statistics() const;
};

#endif  // SEARCH_GROUNDED_GROUNDED_SEARCH_H
