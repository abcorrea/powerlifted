#ifndef SEARCH_SUCCESSOR_GENERATORS_GROUND_SUCCESSOR_GENERATOR_H
#define SEARCH_SUCCESSOR_GENERATORS_GROUND_SUCCESSOR_GENERATOR_H

#include <cstddef>
#include <cstdint>
#include <vector>

class GroundTask;

/**
 * @brief Applicable operators and successor states of the ground task
 * (including the evaluation of its axioms). States are bitsets (see
 * states/ground_state.h).
 *
 * Each operator is watched by the precondition least likely to be true (the
 * one whose predicate has the smallest fraction of true atoms in the initial
 * state, then the one occurring in the fewest preconditions). The operators
 * watched by a fact are grouped by a second precondition, so that only the
 * groups whose second fact is true in the state are scanned.
 */
class GroundSuccessorGenerator {
    const GroundTask &task;
    int num_words;

    // Preconditions of the operators: operator o has the preconditions
    // pre[pre_start[o] .. pre_start[o + 1]) (same layout for the others).
    std::vector<int> pre_start, pre;
    std::vector<int> negated_pre_start, negated_pre;
    // Operators watched by each fact.
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
    std::vector<int> group_mask_start;
    std::vector<uint64_t> group_masks;
    //! Per group: a negated precondition of all its operators, or -1.
    std::vector<int> group_blocker;
    //! Preconditions still to check for the operators in group_ops (aligned
    //! with group_ops; negated precondition f encoded as -1 - f).
    std::vector<int> group_literal_start, group_literals;
    std::vector<int> ops_without_pre;

    // Axioms, sorted by stratum.
    bool has_axioms;
    std::vector<int> stratum_start;
    std::vector<int> axiom_body_start, axiom_body;
    std::vector<int> axiom_negated_body_start, axiom_negated_body;
    std::vector<int> axiom_head;
    std::vector<int> axiom_stratum;
    //! Axioms having each fact in their (positive) body.
    std::vector<int> axioms_by_body_fact_start, axioms_by_body_fact;
    //! Derived facts (heads of axioms).
    std::vector<uint64_t> derived_mask;

    // Scratch data.
    std::vector<int> axiom_num_unsatisfied;
    std::vector<int> derived_queue;
    std::vector<int> fired_effects;

    void build_watch_index();

public:
    explicit GroundSuccessorGenerator(const GroundTask &task);

    //! Only generate the operators marked in `allowed`.
    void restrict_operators(const std::vector<char> &allowed);

    //! Applicable operators (stops after max_ops of them).
    void
    get_applicable(const uint64_t *bits, std::vector<int> &ops, size_t max_ops = SIZE_MAX) const;

    bool is_applicable(const uint64_t *bits, int op) const;

    /**
     * Compute the successor of `parent` by `op` into `child`; the facts
     * true in the child but not in the parent are appended to `added`, the
     * facts true in the parent but not in the child to `removed`.
     */
    void apply(const uint64_t *parent,
               int op,
               uint64_t *child,
               std::vector<int> &added,
               std::vector<int> &removed);

    //! Set the derived facts of the state (all axioms, stratum by stratum).
    void evaluate_axioms(uint64_t *bits);
};

#endif  // SEARCH_SUCCESSOR_GENERATORS_GROUND_SUCCESSOR_GENERATOR_H
