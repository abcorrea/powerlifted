#ifndef SEARCH_DATALOG_TRANSFORMATIONS_STATIC_STRATUM_H_
#define SEARCH_DATALOG_TRANSFORMATIONS_STATIC_STRATUM_H_

#include "../datalog.h"

#include "../grounder/weighted_grounder.h"

#include <unordered_set>
#include <vector>

namespace datalog {

/*
 * Materialize the static stratum: this transformation evaluates once, at
 * initialization, every rule whose extension cannot depend on the state,
 * moves the derived atoms into the permanent EDB with their weighted costs,
 * and removes the rules. Every ground() call then starts from the
 * materialized atoms instead of re-deriving them; profiling showed that the
 * grounder re-derives the static type-joins of the normal form identically
 * on every single state evaluation.
 *
 * A predicate is *static* iff no state fact can ever reach it. The parser
 * routes tuples of static task predicates into the static info and never
 * into states (nullary atoms always live in states, so nullary predicates
 * count as state-fed), and taint propagates through the rules: a head is
 * tainted if any body atom's predicate is tainted. To keep the rewrite
 * exactly behavior-preserving, every materialized rule must also satisfy:
 * - its head is a new (auxiliary) predicate symbol, because atoms of task
 *   predicates feed useful_atoms during backchaining and EDB facts do not;
 * - it carries no annotation, because annotations (e.g. the ff heuristic's
 *   relaxed-plan extraction) execute during backchaining and backchaining
 *   never enters EDB facts.
 * A head with any ineligible rule counts as tainted, so a predicate is
 * materialized only with its full extension and final costs.
 *
 * The weighted grounder itself evaluates the stratum on the reduced rule set
 * (EDB facts only, no goal), with the same cost aggregation as the consuming
 * heuristic, so the materialized costs equal the costs the grounder would
 * have computed per state.
 */
void Datalog::materialize_static_stratum(const Task &task, int heuristic_type) {
    // Taint: predicates whose extension can vary with the state.
    std::unordered_set<int> tainted;
    for (size_t i = 0; i < task.predicates.size(); ++i) {
        if (!task.predicates[i].isStaticPredicate() || task.predicates[i].getArity() == 0) {
            tainted.insert(i);
        }
    }
    // Heads of rules that may never be materialized taint their predicate.
    bool any_annotation = false;
    for (const auto &rule : rules) {
        if (!rule->get_effect().is_pred_symbol_new() || rule->has_annotation()) {
            tainted.insert(rule->get_effect().get_predicate_index());
        }
        if (rule->has_annotation()) any_annotation = true;
    }
    // Annotations (relaxed-plan extraction) recover projected-away variables
    // by descending into the *achievers* of body atoms marked as indirect in
    // the variable-source tables. A materialized fact has no achievers, so
    // any predicate whose sub-derivation that recovery may enter must stay
    // live.
    if (any_annotation) {
        for (const auto &rule : rules) {
            for (const auto &entry :
                 rule->get_variable_source_object_by_ref().get_table()) {
                if (entry.first >= 0 &&
                    entry.first < int(rule->get_conditions().size())) {
                    tainted.insert(
                        rule->get_conditions()[entry.first].get_predicate_index());
                }
            }
        }
    }
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto &rule : rules) {
            int head = rule->get_effect().get_predicate_index();
            if (tainted.count(head)) continue;
            for (const DatalogAtom &b : rule->get_conditions()) {
                if (tainted.count(b.get_predicate_index())) {
                    tainted.insert(head);
                    changed = true;
                    break;
                }
            }
        }
    }

    std::vector<std::unique_ptr<RuleBase>> kept;
    std::vector<std::unique_ptr<RuleBase>> stratum;
    for (auto &rule : rules) {
        if (tainted.count(rule->get_effect().get_predicate_index())) {
            kept.push_back(std::move(rule));
        }
        else {
            stratum.push_back(std::move(rule));
        }
    }

    if (stratum.empty()) {
        rules = std::move(kept);
        return;
    }

    // Evaluate the stratum with the engine itself: EDB facts only, no goal.
    rules = std::move(stratum);
    update_rule_indices();
    {
        WeightedGrounder grounder(*this, heuristic_type);
        std::vector<Fact> no_state_facts;
        grounder.ground(*this, no_state_facts, -1);
        for (auto &rule : rules) {
            rule->clean_up();
        }
    }
    const size_t num_seed_facts = permanent_edb.size();
    const size_t num_materialized = facts.size() - num_seed_facts;
    for (size_t i = num_seed_facts; i < facts.size(); ++i) {
        const Fact &f = facts[i];
        permanent_edb.emplace_back(f.get_arguments(), f.get_predicate_index(), f.get_cost(),
                                   f.is_pred_symbol_new());
    }
    reset_facts();

    std::cout << "Materialized static stratum: " << num_materialized << " atoms from "
              << rules.size() << " rules" << std::endl;

    rules = std::move(kept);
}

}

#endif //SEARCH_DATALOG_TRANSFORMATIONS_STATIC_STRATUM_H_
