#include "weighted_grounder.h"

#include "grounder_statistics.h"

#include "../datalog.h"

#include "../rules/join.h"
#include "../rules/product.h"
#include "../rules/project.h"

#include <deque>
#include <limits>
#include <vector>

using namespace std;

namespace datalog {

int WeightedGrounder::ground(Datalog &datalog, std::vector<Fact> &state_facts, int goal_predicate) {
    phmap::flat_hash_set<Fact> reached_facts;
    // Pre-size from the previous call to skip the doubling rehashes this set
    // would otherwise pay growing from empty (behaviour-neutral: the set is only
    // probed by key, never iterated, so capacity does not affect productions).
    reached_facts.reserve(reached_size_hint);

    queue_pushes = 0;
    atoms_produced = 0;

    // Reset of data structures
    Fact::next_fact_index = 0;

    q.clear();
    best_achievers.clear();
    initial_facts.clear();

    for (const Fact &f : datalog.get_permanent_edb()) {
        Fact f2 = f;
        f2.set_fact_index();
        q.push(f.get_cost(), f2.get_fact_index());
        queue_pushes++;
        atoms_produced++;
        initial_facts.insert(f2.get_fact_index());
        datalog.insert_fact(f2);
        reached_facts.insert(f2);
    }

    for (Fact &f : state_facts) {
        f.set_fact_index();
        q.push(f.get_cost(), f.get_fact_index());
        queue_pushes++;
        atoms_produced++;
        initial_facts.insert(f.get_fact_index());
        datalog.insert_fact(f);
        reached_facts.insert(f);
    }

    while (!q.empty()) {
        pair<int, int> queue_top = q.pop();
        int cost = queue_top.first;
        int top_fact_index = queue_top.second;
        const Fact current_fact = datalog.get_fact_by_index(top_fact_index);
        if (current_fact.get_predicate_index() == goal_predicate) {
            datalog.backchain_from_goal(current_fact, initial_facts);
            record_grounder_run(atoms_produced, queue_pushes);
            reached_size_hint = reached_facts.size();
            return current_fact.get_cost();
        }
        if (current_fact.get_cost() < cost) {
            continue;
        }
        int predicate_index = current_fact.get_predicate_index();
        for (const auto
                &m : rule_matcher.get_matched_rules(predicate_index)) {
            int rule_index = m.get_rule();
            int position_in_the_body = m.get_position();
            RuleBase &rule = datalog.get_rule_by_index(rule_index);

            assert(rule.get_type()==PROJECT || rule.get_type() == JOIN || rule.get_type() == PRODUCT);

            // Each rule produces its head atoms and commits them (probe + queue
            // push) inline, so the achiever for a discarded production is never
            // built. See process_new_fact.
            if (rule.get_type()==PROJECT) {
                // Projection rule - single condition in the body
                assert(position_in_the_body==0);
                project(rule, current_fact, reached_facts, datalog);
            } else if (rule.get_type()==JOIN) {
                // Join rule - two conditions in the body
                assert(position_in_the_body <= 1);
                join(rule, current_fact, position_in_the_body, reached_facts, datalog);
            } else {
                // Product rule - more than one condition without shared free vars
                product(rule, current_fact, position_in_the_body, reached_facts, datalog);
            }
        }
    }
    record_grounder_run(atoms_produced, queue_pushes);
    reached_size_hint = reached_facts.size();
    return std::numeric_limits<int>::max();
}

/*
 * Probe reached_facts with the head atom and commit it iff it is new or
 * strictly cheaper, pushing the kept fact onto the queue. The achiever is
 * produced lazily via build_achievers() and ONLY on the new-fact branch:
 *
 *  - new fact      -> record achiever (it feeds backchaining via lp) + queue.
 *  - cheaper path  -> only the cost matters; lp keeps the first achiever and the
 *                     reached_facts achiever is never read, so we build none.
 *  - has-cheaper   -> discarded; build nothing.
 *
 * Fact equality is predicate+arguments only (see datalog_fact.h), so the probe
 * needs no achiever. This skips ~half of all achiever allocations, which were
 * built and immediately thrown away in the previous batch-then-filter design.
 */
template <typename BuildAchievers>
void WeightedGrounder::process_new_fact(Fact &new_fact,
                                        phmap::flat_hash_set<Fact> &reached_facts,
                                        Datalog &lp,
                                        BuildAchievers &&build_achievers) {
    atoms_produced++;
    const auto it = reached_facts.find(new_fact);
    if (it == reached_facts.end()) {  // The fact wasn't reached yet
        new_fact.update_achievers(build_achievers());
        new_fact.set_fact_index();
        lp.insert_fact(new_fact);
        q.push(new_fact.get_cost(), new_fact.get_fact_index());
        queue_pushes++;
        reached_facts.insert(std::move(new_fact));
    } else if (new_fact.get_cost() < it->get_cost()) {
        new_fact.update_fact_index(it->get_fact_index());
        reached_facts.erase(it);
        lp.update_fact_cost(new_fact.get_fact_index(), new_fact.get_cost());
        q.push(new_fact.get_cost(), new_fact.get_fact_index());
        queue_pushes++;
        reached_facts.insert(std::move(new_fact));
    }
    // else: a cheaper path already exists -- discard, building no achiever.
}

/*
 * Project a sequence of objects into another sequence. The head of the rule H
 * has free(H) <= free(B), where B is the rule body (condition).
 *
 * First, we map every (negative) index in the head to its given position.
 * Then, we loop over the single atom in the body and project the ones
 * that are in the head.  If there are constants in the head, we keep them
 * in the resulting fact when we create the mapping.
 *
 */

void WeightedGrounder::project(const RuleBase &rule_, const Fact &fact,
                               phmap::flat_hash_set<Fact> &reached_facts, Datalog &lp) {
    const ProjectRule &rule = static_cast<const ProjectRule &>(rule_);

    // New arguments start as a copy of the head atom and we just replace the
    // free variables. Constants will remain intact.
    Arguments new_arguments = rule.get_effect_arguments();

    const Arguments &args = rule.get_condition_arguments();
    for (size_t i = 0; i < args.size(); ++i) {
        const auto a = args[i];
        if (args.is_object(i)) {
            // Constant instead of free var
            if (fact.argument(i)!=a) {
                // constants do not match!
                return;
            }
        } else {
            int pos = rule.get_head_position_of_arg(a);
            if (pos!=-1) {
                // Variable should NOT be projected away by this rule
                new_arguments.set_term_to_object(pos, fact.argument(i).get_index());
            }
        }
    }

    Fact new_fact(std::move(new_arguments),
                  rule.get_effect().get_predicate_index(),
                  rule_.get_weight() + fact.get_cost(),
                  rule.get_effect().is_pred_symbol_new());
    process_new_fact(new_fact, reached_facts, lp, [&]() {
        return Achievers(fact.get_fact_index(), rule.get_index(), rule_.get_weight());
    });
}

/*
 * Compute the new facts produced by a join rule.
 *
 * The function starts by computing the fact restricted to the key elements
 * (i.e., elements that the free var matches with the other condition). Then,
 * it updates the hash tables.
 *
 * Next, it maps every free variable to its position in the head atom, similarly
 * as done in the projection, but without considering constants in the head
 * because these should not happen. (I guess.)
 *
 * Then, it computes the new ground head atom by performing first creating
 * the new atom with the values from the currently fact being expanded. Then,
 * it loops over all previously expanded facts matching the same key (the ones
 * in the hash-table) and completing the instantiation.
 *
 * The function returns a list of actions.
 *
 */
void WeightedGrounder::join(
        RuleBase &rule_, const Fact &fact, int position,
        phmap::flat_hash_set<Fact> &reached_facts, Datalog &lp) {
    JoinRule &rule = static_cast<JoinRule &>(rule_);

    JoinHashKey key;
    key.reserve(rule.get_number_joining_vars());
    for (int i : rule.get_position_of_matching_vars(position)) {
        key.push_back(fact.argument(i).get_index());
    }

    // Insert the fact in the hash table of the key
    rule.insert_fact_in_hash(fact, key, position);

    // See comment in "project" about 'new_arguments' vector
    Arguments new_arguments_persistent = rule.get_effect_arguments();

    int position_counter = 0;
    for (auto &arg : rule.get_condition_arguments(position)) {
        int pos = rule.get_head_position_of_arg(arg);
        if (pos!=-1 and !arg.is_object()) {
            new_arguments_persistent.set_term_to_object(pos,
                                                        fact.argument(position_counter).get_index());
        }
        position_counter++;
    }

    int rule_index = rule.get_index();
    int rule_weight = rule.get_weight();
    int pred_index = rule.get_effect().get_predicate_index();
    bool pred_is_new = rule.get_effect().is_pred_symbol_new();
    const int inverse_position = rule.get_inverse_position(position);
    for (const Fact &already_achieved_fact : rule.get_facts_matching_key(key, inverse_position)) {
        Arguments new_arguments = new_arguments_persistent;
        position_counter = 0;
        for (auto &arg : rule.get_condition_arguments(inverse_position)) {
            int pos = rule.get_head_position_of_arg(arg);
            if (pos!=-1 and !arg.is_object()) {
                new_arguments.set_term_to_object(pos,
                                                 already_achieved_fact.argument(position_counter).get_index());
            }
            position_counter++;
        }

        int cost = aggregation_function(fact.get_cost(), already_achieved_fact.get_cost()) + rule_weight;
        Fact new_fact(std::move(new_arguments), pred_index, cost, pred_is_new);
        // Achiever body keeps the atom order of the rule body, so the second
        // body atom (position 1) goes last.
        process_new_fact(new_fact, reached_facts, lp, [&]() {
            if (position == 1) {
                return Achievers(already_achieved_fact.get_fact_index(), fact.get_fact_index(),
                                 rule_index, rule_weight);
            }
            return Achievers(fact.get_fact_index(), already_achieved_fact.get_fact_index(),
                             rule_index, rule_weight);
        });
    }
}

/*
 * In product rules, none of the free variables join and there might be
 * several atoms in the body.
 *
 * In practice, this means two scenarios:
 *
 * (1) the head is empty;
 * (2) every free variable in the body is also in the head
 *
 */
void WeightedGrounder::product(
        RuleBase &rule_, const Fact &fact, int position,
        phmap::flat_hash_set<Fact> &reached_facts, Datalog &lp) {
    ProductRule &rule = static_cast<ProductRule &>(rule_);

    const auto& args = rule.get_condition_arguments(position);

    // Verify that if there is a ground object in the condition of this atom,
    // then it matches the fact being expanded
    int c = 0;
    for (const auto& term:args) {
        if (term.is_object() and term.get_index()!=fact.argument(c).get_index()) {
            return;
        }
        ++c;
    }

    // Check that *all* other positions of the effect have at least one tuple
    rule.add_reached_fact_to_condition(fact, position, fact.get_cost());
    int total_cost = 0;
    std::vector<int> nullary_head_achievers;
    for (const ReachedFacts &v : rule.get_reached_facts_all_conditions()) {
        if (v.empty()) return;
        int min_cost = std::numeric_limits<int>::max();
        int min_index = 0;
        int index = 0;
        for (int cost : v.get_costs()) {
            if (min_cost > cost) {
                min_cost = cost;
                min_index = index;
            }
            index++;
        }
        nullary_head_achievers.push_back(v.get_fact_index(min_index));
        total_cost = aggregation_function(total_cost, min_cost);
    }

    int rule_index = rule.get_index();
    int rule_weight = rule.get_weight();
    int pred_index = rule.get_effect().get_predicate_index();
    bool pred_is_new = rule.get_effect().is_pred_symbol_new();

    // If there is one reachable ground atom for every condition and the head
    // is nullary or has no free variable, then simply trigger it.
    if (rule.head_is_ground()) {
        Fact new_fact(rule.get_effect_arguments(), pred_index,
                      total_cost + rule_weight, pred_is_new);
        process_new_fact(new_fact, reached_facts, lp, [&]() {
            return Achievers(std::move(nullary_head_achievers), rule_index, rule_weight);
        });
        return;
    }

    // Second: start creating a base for the new effect atom based on the fact
    // that we are currently expanding

    // See comment in "project" about 'new_arguments' vector
    Arguments new_arguments_persistent = rule.get_effect_arguments();

    int position_counter = 0;
    for (const auto& arg:args) {
        if (arg.is_object()) continue;
        int pos = rule.get_head_position_of_arg(arg);
        if (pos!=-1) {
            new_arguments_persistent.set_term_to_object(pos,
                                                        fact.argument(position_counter).get_index());
        }
        position_counter++;
    }

    // Third: in this case, we just loop over the other conditions and its already
    // reached facts and instantiate all possibilities (i.e., cartesian product).
    // We do this using a queue (named 'work' so it does not shadow the member
    // priority queue 'q').
    deque<ProductDequeEntry> work;
    work.emplace_back(new_arguments_persistent, 0, fact.get_cost());
    while (!work.empty()) {
        auto& next = work.front();

        if (next.index >= int(rule.get_conditions().size())) {
            Fact new_fact(next.arguments, pred_index, next.cost + rule_weight, pred_is_new);
            std::vector<int> achievers_body = std::move(next.achiever_atoms_indices);
            process_new_fact(new_fact, reached_facts, lp, [&]() {
                return Achievers(std::move(achievers_body), rule_index, rule_weight);
            });
        } else if (next.index==position) {
            // If it is the condition that we are currently reaching, we do not need
            // to consider the other tuples with this predicate
            next.achiever_atoms_indices.push_back(fact.get_fact_index());
            work.emplace_back(next.arguments, next.index + 1, next.cost, next.achiever_atoms_indices);
        } else {
            int vector_counter = 0;
            for (const auto &assignment : rule.get_reached_facts_of_condition(next.index)) {
                Arguments new_arguments = next.arguments; // start as a copy
                size_t value_counter = 0;
                for (const Term &term : rule.get_condition_arguments(next.index)) {
                    assert (value_counter < assignment.size());
                    int pos = rule.get_head_position_of_arg(term);
                    if (pos!=-1) {
                        new_arguments.set_term_to_object(pos,
                                                         assignment[value_counter].get_index());
                    }
                    ++value_counter;
                }
                std::vector<int> new_achievers = next.achiever_atoms_indices;
                new_achievers.push_back(rule.get_fact_index_reached_fact_in_position(next.index, vector_counter));
                work.emplace_back(
                        std::move(new_arguments),
                        next.index + 1,
                        aggregation_function(next.cost, rule.get_cost_reached_fact_in_position(next.index, vector_counter++)),
                        std::move(new_achievers));
            }
        }
        work.pop_front();
    }
}

void WeightedGrounder::create_rule_matcher(const Datalog &lp) {
    // Loop over rule conditions
    for (const auto &rule : lp.get_rules()) {
        int cont = 0;
        for (const auto &condition : rule->get_conditions()) {
            rule_matcher.insert(condition.get_predicate_index(),
                                rule->get_index(),
                                cont++);
        }
    }
}

}