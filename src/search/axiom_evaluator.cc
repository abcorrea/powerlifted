#include "axiom_evaluator.h"

#include "database/hash_join.h"

#include <algorithm>
#include <cassert>

using namespace std;

AxiomEvaluator::AxiomEvaluator(const vector<Axiom> &axioms,
                               int number_of_strata,
                               const vector<Predicate> &predicates,
                               const StaticInformation &static_information)
{
    if (axioms.empty()) return;

    for (const Predicate &p : predicates) {
        if (p.isDerivedPredicate()) {
            derived_predicates.push_back(p.get_index());
        }
    }

    // Same criterion as GenericJoinSuccessor: an atom's table is precompiled
    // iff its relation lives in the static information. (A static predicate
    // with an empty extension counts as fluent; its state relation is empty
    // as well, so the body simply never matches.)
    vector<bool> is_static;
    is_static.reserve(static_information.get_relations().size());
    for (const auto &r : static_information.get_relations()) {
        is_static.push_back(!r.tuples.empty());
    }

    assert(number_of_strata > 0);
    axioms_by_stratum.resize(number_of_strata);
    for (const Axiom &axiom : axioms) {
        assert(axiom.get_stratum() >= 0 && axiom.get_stratum() < number_of_strata);
#ifndef NDEBUG
        // Derived body atoms must never be treated as static: they are
        // recomputed for every state.
        for (const Atom &a : axiom.get_body()) {
            assert(!predicates[a.get_predicate_symbol_idx()].isDerivedPredicate()
                   || !is_static[a.get_predicate_symbol_idx()]);
        }
#endif
        PrecompiledAxiom pax(axiom);
        pax.program = join_program::precompile(
            vector<Atom>(axiom.get_body()), is_static, static_information);
        axioms_by_stratum[axiom.get_stratum()].push_back(std::move(pax));
    }
}

void AxiomEvaluator::evaluate(DBState &state) const
{
    if (!has_axioms()) return;

    // Discard the derived atoms inherited from the parent state (or, for
    // the initial state, be robust against leftovers): the extension of a
    // derived predicate is a function of the base fluents and is recomputed
    // from scratch.
    for (int pred : derived_predicates) {
        state.clear_relation(pred);
        // For nullary derived predicates the truth value lives in the
        // nullary-atom vector; for the others this is a no-op.
        state.set_nullary_atom(pred, false);
    }

    for (const auto &stratum : axioms_by_stratum) {
        // Naive fixpoint: re-evaluate every axiom of the stratum until no
        // rule derives a new atom. (Semi-naive evaluation is a documented
        // possible follow-up.)
        bool changed = true;
        while (changed) {
            changed = false;
            for (const PrecompiledAxiom &pax : stratum) {
                if (evaluate_axiom(pax, state)) {
                    changed = true;
                }
            }
        }
    }
}

bool AxiomEvaluator::evaluate_axiom(const PrecompiledAxiom &pax,
                                    DBState &state) const
{
    const Axiom &axiom = pax.axiom;
    const int head = axiom.get_head_predicate();
    const int arity = axiom.get_head_arity();

    for (int pred : axiom.get_positive_nullary_body()) {
        if (!state.get_nullary_atoms()[pred]) return false;
    }

    if (arity == 0 && state.get_nullary_atoms()[head]) {
        // Nullary head already derived; nothing new can come out of this
        // axiom.
        return false;
    }

    if (pax.program.relevant_atoms.empty()) {
        // The body has no atoms with arguments. Non-nullary heads always
        // have their parameters' type atoms in the body, so the head must be
        // nullary; only constant-only equality literals can remain.
        assert(arity == 0);
        for (const Atom &eq : axiom.get_equalities()) {
            const auto &args = eq.get_arguments();
            assert(args.size() == 2);
            assert(args[0].is_constant() && args[1].is_constant());
            bool is_equal = (args[0].get_index() == args[1].get_index());
            if (eq.is_negated() == is_equal) return false;
        }
        state.set_nullary_atom(head, true);
        return true;
    }

    vector<Table> tables;
    if (!join_program::fill_tables(pax.program, state, tables)) return false;

    Table working_table = std::move(tables[0]);
    vector<bool> applied(axiom.get_equalities().size(), false);
    join_program::filter_equalities(axiom.get_equalities(), working_table,
                                    applied);
    if (working_table.tuples.empty()) return false;
    for (size_t i = 1; i < tables.size(); ++i) {
        hash_join(working_table, tables[i]);
        join_program::filter_equalities(axiom.get_equalities(), working_table,
                                        applied);
        if (working_table.tuples.empty()) return false;
    }

    if (arity == 0) {
        state.set_nullary_atom(head, true);
        return true;
    }

    // Project the answers onto the head parameters (indices 0..arity-1;
    // every parameter occurs in the body thanks to the type atoms the
    // translator adds).
    vector<int> head_columns(arity, -1);
    const auto &tuple_index = working_table.tuple_index;
    for (int i = 0; i < arity; ++i) {
        auto it = find(tuple_index.begin(), tuple_index.end(), i);
        assert(it != tuple_index.end());
        head_columns[i] = distance(tuple_index.begin(), it);
    }

    bool changed = false;
    for (const auto &tuple : working_table.tuples) {
        GroundAtom ga;
        ga.reserve(arity);
        for (int c : head_columns) {
            ga.push_back(tuple[c]);
        }
        if (state.insert_tuple_if_new(std::move(ga), head)) {
            changed = true;
        }
    }
    return changed;
}
