#ifndef SEARCH_GROUNDING_FAST_GROUNDER_H
#define SEARCH_GROUNDING_FAST_GROUNDER_H

#include "../structures.h"

#include <memory>
#include <utility>
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
/**
 * @brief Decomposed relaxed program of the action schemas, computed when
 * there are too many ground actions to ground them (see
 * compute_relaxed_reachability()).
 *
 * The rules come from splitting each schema (and conditional effect) by
 * variable elimination, per group of effects with the same variables;
 * auxiliary rules have cost 0, final rules derive the effects at the cost of
 * the action. The instances are the rules' groundings over the relaxed
 * reachable atoms. An atom (r, args) is an atom of task predicate r if
 * 0 <= r < number of task predicates, the negation of predicate -r - 1 if
 * r < 0, and an auxiliary atom otherwise.
 */
struct RelaxedProgram {
    using ProgramAtom = std::pair<int, GroundAtom>;
    struct Rule {
        int schema;
        int cost;
        bool final;
        //! Variables bound by an instance, in increasing order (the schema
        //! parameters are the variables below the number of parameters).
        std::vector<int> vars;
    };
    struct Instance {
        int rule;
        std::vector<ProgramAtom> body;
        std::vector<ProgramAtom> heads;
        //! Values of the rule's variables.
        GroundAtom values;
    };
    std::vector<Rule> rules;
    std::vector<Instance> instances;
};

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
    //! Count-first grounding (the default): number of ground actions
    //! counted before enumerating them, and whether they exceed the limit.
    double counted_operators = -1;
    bool too_many_operators = false;
    //! If there are too many operators: the decomposed relaxed program (if
    //! it is small enough), and the atoms and axiom instances are complete.
    std::shared_ptr<RelaxedProgram> relaxed_program;
};

/**
 * Compute the relaxed-reachable instances. Returns false (and leaves the
 * result incomplete) if more than `max_instances` atoms and instances are
 * produced in total.
 *
 * By default (unless PWL_NO_SPLIT_GROUNDING), the fixpoint derives no instance atoms: its
 * rules are split by variable elimination (with auxiliary relations), so it
 * costs about as much as the reachable atoms. Then the ground actions are
 * counted by variable elimination; if there are more than `max_operators`,
 * the result is incomplete (too_many_operators). Otherwise the action,
 * conditional effect, and axiom instances are enumerated with one join each
 * over the reachable atoms.
 */
bool compute_relaxed_reachability(const Task &task, size_t max_instances,
                                  size_t max_operators, RelaxedReachabilityResult &result);

#endif  // SEARCH_GROUNDING_FAST_GROUNDER_H
