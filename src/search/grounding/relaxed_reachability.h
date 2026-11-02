#ifndef SEARCH_GROUNDING_RELAXED_REACHABILITY_H
#define SEARCH_GROUNDING_RELAXED_REACHABILITY_H

#include "../structures.h"

#include <memory>
#include <utility>
#include <vector>

class Task;

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
    //! Number of ground actions, counted before enumerating them.
    double counted_operators = -1;
    //! If there are too many operators: the decomposed relaxed program (if
    //! it is small enough); the atoms and axiom instances are complete.
    std::shared_ptr<RelaxedProgram> relaxed_program;
};

enum class ReachabilityOutcome {
    COMPLETE,
    //! More atoms and instances than allowed.
    TOO_MANY_ATOMS,
    //! More ground operators than allowed: only the atoms, the axiom
    //! instances, and the relaxed program (if small enough) are computed.
    TOO_MANY_OPERATORS,
    //! Out of time.
    TIMED_OUT
};

/**
 * Compute the atoms and the ground instances of all action schemas,
 * conditional effects, and axioms reachable in the delete relaxation from
 * the initial state, within the limits: at most `max_instances` atoms and
 * instances, at most `max_operators` ground operators, and at most
 * `max_seconds` of wall-clock time.
 *
 * The fixpoint derives no instance atoms: its rules are split by variable
 * elimination (with auxiliary relations), so it costs about as much as the
 * reachable atoms. Then the ground actions are counted by variable
 * elimination, and only if there are not too many, the action, conditional
 * effect, and axiom instances are enumerated with one join each over the
 * reachable atoms.
 */
ReachabilityOutcome compute_relaxed_reachability(const Task &task,
                                                 size_t max_instances,
                                                 size_t max_operators,
                                                 double max_seconds,
                                                 RelaxedReachabilityResult &result);

#endif  // SEARCH_GROUNDING_RELAXED_REACHABILITY_H
