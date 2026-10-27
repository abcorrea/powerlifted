#ifndef SEARCH_GROUNDING_FAST_GROUNDER_H
#define SEARCH_GROUNDING_FAST_GROUNDER_H

#include "../structures.h"

#include <vector>

class Task;

/**
 * @brief Relaxed reachability analysis computing the ground instances of all
 * action schemas, conditional effects, and axioms reachable in the delete
 * relaxation from the initial state.
 *
 * Every schema, conditional effect, and axiom is a rule whose body is its
 * positive (non-'=') condition atoms and whose head is an instance atom
 * listing all its variables; (in)equalities and negated static atoms are
 * checked as soon as their variables are bound. Rules are evaluated
 * semi-naively: every new atom is joined, in each body position it matches,
 * with the atoms reached so far, by backtracking over the remaining body
 * atoms in a fixed order using hash indices on the bound argument positions.
 * No intermediate join results are materialized.
 *
 * Variables of positive equalities between two variables are unified before
 * evaluation.
 */
struct RelaxedReachabilityResult {
    //! Reached atoms per task predicate (fluent and static; nullary
    //! predicates have the empty tuple if reached).
    std::vector<std::vector<GroundAtom>> atoms;
    //! Instances per action schema: values of all parameters.
    std::vector<std::vector<GroundAtom>> action_instances;
    //! Instances per action schema and conditional effect: values of the
    //! action parameters followed by the quantified variables.
    std::vector<std::vector<std::vector<GroundAtom>>> ce_instances;
    //! Instances per axiom: values of all axiom parameters.
    std::vector<std::vector<GroundAtom>> axiom_instances;
};

/**
 * Compute the relaxed-reachable instances. Returns false (and leaves the
 * result incomplete) if more than `max_instances` atoms and instances are
 * produced in total.
 */
bool compute_relaxed_reachability(const Task &task, size_t max_instances,
                                  RelaxedReachabilityResult &result);

#endif  // SEARCH_GROUNDING_FAST_GROUNDER_H
