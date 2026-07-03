#include "weighted_grounder.h"

#include "../datalog.h"

#include "../rules/join.h"
#include "../rules/product.h"
#include "../rules/project.h"

#include <deque>
#include <limits>
#include <unordered_set>
#include <vector>

using namespace std;

namespace datalog {

/*
 * Exact inside (cheapest derivation) and outside (cheapest completion of a
 * goal derivation) costs of the predicate-level abstraction. The abstract
 * hypergraph has one node per predicate and one hyperedge per rule; both
 * fixpoints are Bellman-Ford style loops over the rules, which is plenty on a
 * graph this small (costs are monotone and integral, so they terminate).
 */
void WeightedGrounder::compute_abstract_costs(Datalog &datalog,
                                              const std::vector<Fact> &state_facts,
                                              int goal_predicate) {
    size_t num_preds = 0;
    for (const auto &rule : datalog.get_rules()) {
        num_preds = std::max(num_preds, size_t(rule->get_effect().get_predicate_index()) + 1);
        for (const DatalogAtom &b : rule->get_conditions()) {
            num_preds = std::max(num_preds, size_t(b.get_predicate_index()) + 1);
        }
    }
    for (const Fact &f : datalog.get_permanent_edb()) {
        num_preds = std::max(num_preds, size_t(f.get_predicate_index()) + 1);
    }
    for (const Fact &f : state_facts) {
        num_preds = std::max(num_preds, size_t(f.get_predicate_index()) + 1);
    }
    num_preds = std::max(num_preds, size_t(goal_predicate) + 1);

    abstract_inside.assign(num_preds, ABSTRACT_INF);
    abstract_outside.assign(num_preds, ABSTRACT_INF);

    // Base: the cheapest initial fact of each predicate. The persistent base
    // lives in the fact vector prefix; the improvable EDB facts and the state
    // facts arrive as arguments.
    for (int i = 0; i < num_base_facts; ++i) {
        const Fact &f = datalog.get_fact_by_index(i);
        int &c = abstract_inside[f.get_predicate_index()];
        c = std::min(c, f.get_cost());
    }
    for (const Fact &f : improvable_edb) {
        int &c = abstract_inside[f.get_predicate_index()];
        c = std::min(c, f.get_cost());
    }
    for (const Fact &f : state_facts) {
        int &c = abstract_inside[f.get_predicate_index()];
        c = std::min(c, f.get_cost());
    }

    const auto &rules = datalog.get_rules();
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto &rule : rules) {
            int body_agg = 0;
            bool infinite = false;
            for (const DatalogAtom &b : rule->get_conditions()) {
                int c = abstract_inside[b.get_predicate_index()];
                if (c >= ABSTRACT_INF) {
                    infinite = true;
                    break;
                }
                body_agg = aggregation_function(body_agg, c);
            }
            if (infinite) continue;
            int head_cost = body_agg + rule->get_weight();
            int &c = abstract_inside[rule->get_effect().get_predicate_index()];
            if (head_cost < c) {
                c = head_cost;
                changed = true;
            }
        }
    }

    abstract_outside[goal_predicate] = 0;
    changed = true;
    while (changed) {
        changed = false;
        for (const auto &rule : rules) {
            int out_head = abstract_outside[rule->get_effect().get_predicate_index()];
            if (out_head >= ABSTRACT_INF) continue;
            const std::vector<DatalogAtom> &body = rule->get_conditions();
            for (size_t i = 0; i < body.size(); ++i) {
                int others_agg = 0;
                bool infinite = false;
                for (size_t j = 0; j < body.size(); ++j) {
                    if (j == i) continue;
                    int c = abstract_inside[body[j].get_predicate_index()];
                    if (c >= ABSTRACT_INF) {
                        infinite = true;
                        break;
                    }
                    others_agg = aggregation_function(others_agg, c);
                }
                if (infinite) continue;
                int candidate =
                    aggregation_function(out_head, others_agg + rule->get_weight());
                int &out = abstract_outside[body[i].get_predicate_index()];
                if (candidate < out) {
                    out = candidate;
                    changed = true;
                }
            }
        }
    }
}

int WeightedGrounder::ground(Datalog &datalog, std::vector<Fact> &state_facts, int goal_predicate) {
    std::vector<Fact> newfacts;

    queue_pushes = 0;
    atoms_produced = 0;

    if (!base_initialized) {
        // One-time split of the EDB into the persistent base and the
        // improvable remainder (see header comment).
        std::unordered_set<int> derived_preds;
        for (const auto &rule : datalog.get_rules()) {
            derived_preds.insert(rule->get_effect().get_predicate_index());
        }
        Fact::next_fact_index = 0;
        for (const Fact &f : datalog.get_permanent_edb()) {
            if (derived_preds.count(f.get_predicate_index())) {
                improvable_edb.push_back(f);
            }
            else {
                Fact f2 = f;
                f2.set_fact_index();
                datalog.insert_fact(f2);
            }
        }
        num_base_facts = Fact::next_fact_index;
        base_initialized = true;
    }

    // Reset of data structures: drop everything after the persistent base.
    datalog.truncate_facts(num_base_facts);
    Fact::next_fact_index = num_base_facts;
    reached_facts.clear();

    q.clear();
    best_achievers.clear();

    if (goal_predicate >= 0) {
        compute_abstract_costs(datalog, state_facts, goal_predicate);
    }
    else {
        abstract_outside.clear();
    }

    for (int i = 0; i < num_base_facts; ++i) {
        atoms_produced++;
        cumulative_atoms_produced++;
        int priority = priority_of(datalog.get_fact_by_index(i));
        if (priority >= ABSTRACT_INF) continue;
        q.push(priority, i);
        queue_pushes++;
        cumulative_queue_pushes++;
    }

    for (const Fact &f : improvable_edb) {
        Fact f2 = f;
        f2.set_fact_index();
        atoms_produced++;
        cumulative_atoms_produced++;
        datalog.insert_fact(f2);
        reached_facts.insert(f2);
        int priority = priority_of(f2);
        if (priority >= ABSTRACT_INF) continue;
        q.push(priority, f2.get_fact_index());
        queue_pushes++;
        cumulative_queue_pushes++;
    }

    for (Fact &f : state_facts) {
        f.set_fact_index();
        atoms_produced++;
        cumulative_atoms_produced++;
        datalog.insert_fact(f);
        reached_facts.insert(f);
        int priority = priority_of(f);
        if (priority >= ABSTRACT_INF) continue;
        q.push(priority, f.get_fact_index());
        queue_pushes++;
        cumulative_queue_pushes++;
    }

    // EDB + state facts now own the contiguous index range [0, num_initial_facts).
    num_initial_facts = Fact::next_fact_index;

    while (!q.empty()) {
        pair<int, int> queue_top = q.pop();
        int cost = queue_top.first;
        int top_fact_index = queue_top.second;
        // Access the popped fact by reference, not by copy (a copy clones its
        // arguments and achiever body on every pop). is_cheapest_path_to_achieve_fact()
        // below can push_back to — and thus reallocate — datalog's fact vector, so
        // we never hold this reference across it; we re-fetch by index (O(1))
        // inside the rule loop, where no fact is inserted during a single
        // project/join/product call.
        const Fact &popped_fact = datalog.get_fact_by_index(top_fact_index);
        if (popped_fact.get_predicate_index() == goal_predicate) {
            datalog.backchain_from_goal(popped_fact, num_initial_facts);
            return popped_fact.get_cost();
        }
        if (priority_of(popped_fact) < cost) {
            continue;
        }
        int predicate_index = popped_fact.get_predicate_index();
        for (const auto
                &m : rule_matcher.get_matched_rules(predicate_index)) {
            int rule_index = m.get_rule();
            int position_in_the_body = m.get_position();
            RuleBase &rule = datalog.get_rule_by_index(rule_index);

            assert(rule.get_type()==PROJECT || rule.get_type() == JOIN || rule.get_type() == PRODUCT);

            newfacts.clear();
            // Re-fetch: a previous iteration's is_cheapest_path may have grown
            // (and moved) the fact vector. Valid for this single rule application.
            const Fact &current_fact = datalog.get_fact_by_index(top_fact_index);
            if (rule.get_type()==PROJECT) {
                // Projection rule - single condition in the body
                assert(position_in_the_body==0);
                project(rule, current_fact, newfacts);
            } else if (rule.get_type()==JOIN) {
                // Join rule - two conditions in the body
                assert(position_in_the_body <= 1);
                join(rule, current_fact, position_in_the_body, newfacts);
            } else {
                // Product rule - more than one condition without shared free vars
                product(rule, current_fact, position_in_the_body, newfacts);
            }

            // Note: using for loop for performance reasons, this is a heavily used loop
            for (unsigned i=0, sz=newfacts.size(); i < sz; ++i) {
                auto& new_fact = newfacts[i];
                int id = is_cheapest_path_to_achieve_fact(new_fact, reached_facts, datalog);
                //datalog.output_atom(new_fact);
                //std::cout << std::endl << std::flush;
                if (id!=HAS_CHEAPER_PATH) {
                    // Duplicate detection above already recorded the fact;
                    // queue it only if it can still join a goal derivation.
                    int priority = priority_of(new_fact);
                    if (priority >= ABSTRACT_INF) continue;
                    q.push(priority, id);
                    queue_pushes++;
                    cumulative_queue_pushes++;
                }
            }
        }
    }
    return std::numeric_limits<int>::max();
}

int WeightedGrounder::is_cheapest_path_to_achieve_fact(Fact &new_fact,
                                                       phmap::flat_hash_set<Fact> &reached_facts,
                                                       Datalog &lp) {
    atoms_produced++;
    cumulative_atoms_produced++;
    // Fuse the membership probe and the insert into one hash+probe pass. The old
    // code did find() then, on a miss, a second insert() (two hashes of the whole
    // argument tuple per new fact). lazy_emplace() probes once and constructs the
    // stored copy only when the fact is new.
    bool inserted = false;
    const auto it = reached_facts.lazy_emplace(new_fact, [&](const auto &ctor) {
        new_fact.set_fact_index();
        ctor(new_fact);
        inserted = true;
    });
    if (inserted) {  // The fact wasn't reached yet
        lp.insert_fact(new_fact);
        return new_fact.get_fact_index();
    }
    if (new_fact.get_cost() < it->get_cost()) {
        const int existing_index = it->get_fact_index();
        new_fact.update_fact_index(existing_index);
        // Cost is neither hashed nor compared (Fact keys on predicate+args only),
        // and the stored entry's achiever body is never read back from
        // reached_facts (backchaining reads achievers from Datalog::facts), so
        // lower the cost in place instead of erase+reinsert (one hash saved).
        const_cast<Fact &>(*it).set_cost(new_fact.get_cost());
        lp.update_fact_cost(existing_index, new_fact.get_cost());
        return existing_index;
    }
    return HAS_CHEAPER_PATH;
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

void WeightedGrounder::project(const RuleBase &rule_, const Fact &fact, std::vector<Fact>& newfacts) {
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

    // Return a vector with one single fact. The single-int Achievers ctor keeps
    // the achiever body in the inline small_vector (no heap-allocated vector).
    newfacts.emplace_back(std::move(new_arguments),
                          rule.get_effect().get_predicate_index(),
                          rule_.get_weight() + fact.get_cost(),
                          Achievers(fact.get_fact_index(), rule.get_index(), rule_.get_weight()),
                          rule.get_effect().is_pred_symbol_new());
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
        RuleBase &rule_, const Fact &fact, int position, std::vector<Fact>& newfacts) {
    JoinRule &rule = static_cast<JoinRule &>(rule_);

    // Build the join key in the reused buffer (no per-call allocation).
    JoinHashKey &key = join_key_buffer;
    key.clear();
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

        // Achiever body in rule-body order. If `fact` is the atom in the second
        // position (index 1), swap so the body order is preserved. Constructing
        // the two-int Achievers directly avoids a per-fact heap-allocated
        // std::vector<int> (the inline small_vector<int,2> holds both elements).
        int achiever_first = fact.get_fact_index();
        int achiever_second = already_achieved_fact.get_fact_index();
        if (position == 1) {
            std::swap(achiever_first, achiever_second);
        }

        int cost = aggregation_function(fact.get_cost(), already_achieved_fact.get_cost()) + rule_weight;
        newfacts.emplace_back(std::move(new_arguments),
                           rule.get_effect().get_predicate_index(),
                           cost, Achievers(achiever_first, achiever_second, rule_index, rule_weight),
                           rule.get_effect().is_pred_symbol_new());
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
        RuleBase &rule_, const Fact &fact, int position, std::vector<Fact>& newfacts) {
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
    const std::vector<ReachedFacts> &all_conditions = rule.get_reached_facts_all_conditions();
    for (const ReachedFacts &v : all_conditions) {
        if (v.empty()) return;
    }

    // If there is one reachable ground atom for every condition and the head
    // is nullary or has no free variable, then simply trigger it. Only the
    // cheapest tuple of each condition matters here, and ReachedFacts tracks that
    // minimum incrementally, so this is O(#conditions) instead of re-scanning
    // (and copying) every reached tuple's costs on every call. The min-scan is
    // pointless for non-ground heads, so we skip it entirely in that case.
    if (rule.head_is_ground()) {
        int total_cost = 0;
        std::vector<int> nullary_head_achievers;
        nullary_head_achievers.reserve(all_conditions.size());
        for (const ReachedFacts &v : all_conditions) {
            nullary_head_achievers.push_back(v.get_min_fact_index());
            total_cost = aggregation_function(total_cost, v.get_min_cost());
        }
        newfacts.emplace_back(rule.get_effect_arguments(),
                              rule.get_effect().get_predicate_index(),
                              total_cost + rule.get_weight(),
                              Achievers(std::move(nullary_head_achievers), rule.get_index(), rule.get_weight()),
                              rule.get_effect().is_pred_symbol_new());
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
    // We do this using a queue
    deque<ProductDequeEntry> q;
    //deque<pair<Arguments, int>> q;
    q.emplace_back(new_arguments_persistent, 0, fact.get_cost());
    while (!q.empty()) {
        auto& next = q.front();

        if (next.index >= int(rule.get_conditions().size())) {
            newfacts.emplace_back(next.arguments,
                                  rule.get_effect().get_predicate_index(),
                                  next.cost + rule.get_weight(),
                                  Achievers(next.achiever_atoms_indices, rule.get_index(), rule.get_weight()),
                                  rule.get_effect().is_pred_symbol_new());
        } else if (next.index==position) {
            // If it is the condition that we are currently reaching, we do not need
            // to consider the other tuples with this predicate
            next.achiever_atoms_indices.push_back(fact.get_fact_index());
            q.emplace_back(next.arguments, next.index + 1, next.cost, next.achiever_atoms_indices);
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
                q.emplace_back(
                        std::move(new_arguments),
                        next.index + 1,
                        aggregation_function(next.cost, rule.get_cost_reached_fact_in_position(next.index, vector_counter++)),
                        std::move(new_achievers));
            }
        }
        q.pop_front();
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