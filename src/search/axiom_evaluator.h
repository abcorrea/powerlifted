#ifndef SEARCH_AXIOM_EVALUATOR_H
#define SEARCH_AXIOM_EVALUATOR_H

#include "axiom.h"
#include "predicate.h"

#include "database/join_program.h"
#include "states/state.h"

#include <vector>

/**
 * @brief Computes the derived predicates of a state, stratum by stratum.
 *
 * Each axiom body is evaluated as a conjunctive query with the same
 * precompiled join-program machinery used for action preconditions
 * (database/join_program.h). Within a stratum, the axioms are applied
 * repeatedly until a fixpoint is reached (naive evaluation; semi-naive
 * evaluation is a possible follow-up). Atoms derived in earlier strata are
 * part of the state when later strata are evaluated.
 *
 * Design decision (Option A of the axiom-support plan): derived atoms are
 * stored in the state's ordinary relations, filled in by evaluate() when the
 * state is created (initial state and every successor). Goal checks,
 * applicability tests and heuristic fact extraction then see derived atoms
 * with no call-site changes. The derived relations are a deterministic
 * function of the base fluents, so packing them along with the rest of the
 * state cannot pollute duplicate detection: two states with equal base
 * fluents always carry equal derived atoms, provided every state is
 * evaluated before it is packed — which generate_successor() and the parser
 * (for the initial state) guarantee.
 */
class AxiomEvaluator {

    struct PrecompiledAxiom {
        explicit PrecompiledAxiom(const Axiom &axiom) : axiom(axiom) {}

        Axiom axiom;
        PrecompiledJoinProgram program;
    };

    //! Axioms grouped by stratum: axioms_by_stratum[i] holds the
    //! (precompiled) axioms whose head is in stratum i.
    std::vector<std::vector<PrecompiledAxiom>> axioms_by_stratum;

    //! Predicate indices of all derived predicates (cleared before
    //! re-evaluation).
    std::vector<int> derived_predicates;

    //! Evaluate a single axiom on the state; returns true if a new atom of
    //! the head predicate was derived.
    bool evaluate_axiom(const PrecompiledAxiom &pax, DBState &state) const;

public:
    //! Constructs an evaluator for a task without axioms; evaluate() is a
    //! no-op then.
    AxiomEvaluator() = default;

    AxiomEvaluator(const std::vector<Axiom> &axioms,
                   int number_of_strata,
                   const std::vector<Predicate> &predicates,
                   const StaticInformation &static_information);

    bool has_axioms() const {
        return !axioms_by_stratum.empty();
    }

    //! Recompute the derived relations of the state in place: all derived
    //! atoms are discarded and rederived from the base fluents (and the
    //! static information).
    void evaluate(DBState &state) const;
};

#endif // SEARCH_AXIOM_EVALUATOR_H
