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
 * (database/join_program.h). Within a stratum, the fixpoint is computed by
 * semi-naive evaluation: the first round evaluates every axiom of the
 * stratum over the full relations; every later round re-evaluates only the
 * recursive axioms (those with a body atom whose predicate is derived in
 * the same stratum), once per recursive body position, with that position's
 * table restricted to the delta -- the tuples first derived in the previous
 * round. Any tuple that is new in round i+1 must use at least one tuple of
 * delta i in its body (otherwise it would have been derived in an earlier
 * round), so restricting one position at a time to the delta loses nothing,
 * while derivations from older rounds are not recomputed over and over.
 * Recursive dependencies on *nullary* derived predicates have no table
 * position; an axiom with such a dependency is re-evaluated in full in the
 * round after the nullary atom becomes true. Atoms derived in earlier
 * strata are part of the state when later strata are evaluated.
 *
 * Negated body atoms are evaluated as negation as failure: a tuple
 * survives iff the instantiated atom is absent from its relation. The
 * stratification guarantees that a negated derived predicate lives in a
 * strictly lower stratum, so its extension -- like that of negated fluent
 * and static atoms -- is already complete and constant while the current
 * stratum's fixpoint runs, which also keeps the semi-naive deltas sound.
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

    //! Tuples first derived in the current fixpoint round, per predicate
    //! index. For a nullary head, "the atom became true" is recorded as a
    //! single empty tuple.
    using NewTuples = std::vector<std::vector<GroundAtom>>;

    struct PrecompiledAxiom {
        explicit PrecompiledAxiom(const Axiom &axiom) : axiom(axiom) {}

        Axiom axiom;
        PrecompiledJoinProgram program;

        //! Positions in program.relevant_atoms whose predicate is derived in
        //! the same stratum as the head: the delta positions of the
        //! semi-naive evaluation.
        std::vector<int> recursive_positions;

        //! Nullary body predicates derived in the same stratum as the head;
        //! they have no table position, so the axiom is re-evaluated in full
        //! when one of them becomes true.
        std::vector<int> recursive_nullary;

        bool is_recursive() const {
            return !recursive_positions.empty() || !recursive_nullary.empty();
        }
    };

    struct Stratum {
        std::vector<PrecompiledAxiom> axioms;
        //! Whether any axiom of the stratum is recursive; if none is, a
        //! single evaluation round suffices and no delta is collected.
        bool has_recursive_axiom = false;
    };

    //! Axioms grouped by stratum, in evaluation order.
    std::vector<Stratum> strata;

    //! Predicate indices of all derived predicates (cleared before
    //! re-evaluation).
    std::vector<int> derived_predicates;

    //! Number of predicates of the task (size of the per-predicate delta).
    size_t num_predicates = 0;

    //! Static relations and the per-predicate static classification, needed
    //! to resolve the relations of negated body atoms.
    const StaticInformation *static_information = nullptr;
    std::vector<bool> is_static;

    //! Evaluate a single axiom on the state; returns true if a new atom of
    //! the head predicate was derived. If delta_position >= 0, the body
    //! atom at that position is restricted to delta_tuples (semi-naive
    //! round); newly derived tuples are appended to *new_tuples if it is
    //! non-null.
    bool evaluate_axiom(const PrecompiledAxiom &pax, DBState &state,
                        int delta_position,
                        const std::vector<GroundAtom> *delta_tuples,
                        NewTuples *new_tuples) const;

public:
    //! Constructs an evaluator for a task without axioms; evaluate() is a
    //! no-op then.
    AxiomEvaluator() = default;

    AxiomEvaluator(const std::vector<Axiom> &axioms,
                   int number_of_strata,
                   const std::vector<Predicate> &predicates,
                   const StaticInformation &static_information);

    bool has_axioms() const {
        return !strata.empty();
    }

    //! Recompute the derived relations of the state in place: all derived
    //! atoms are discarded and rederived from the base fluents (and the
    //! static information).
    void evaluate(DBState &state) const;
};

#endif // SEARCH_AXIOM_EVALUATOR_H
