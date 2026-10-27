#include "grounded_heuristic.h"

#include "../grounding/ground_task.h"
#include "../states/state.h"

#include <algorithm>
#include <iostream>
#include <limits>

using namespace std;

static const int INF = numeric_limits<int>::max() / 2;

GroundedHeuristic::GroundedHeuristic(const Task &task, shared_ptr<GroundTask> gt, Type type)
    : type(type), ground_task(std::move(gt)) {
    const GroundTask &g = *ground_task;
    num_base_facts = g.num_facts();
    complement.assign(num_base_facts, -1);
    num_facts = num_base_facts;
    /*
      Complement facts. For a non-derived fact f, not-f is added by the
      operators deleting f. Derived facts are never deleted by operators:
      not-d holds iff the body of every ground axiom for d is false, i.e.,
      iff some literal of each body is false. If this CNF has few
      combinations, each combination (one falsified literal per body) becomes
      a zero-cost relaxed operator achieving not-d (like Fast Downward's
      negated axioms); otherwise the negated derived atom is relaxed away.
    */
    auto is_derived = [&](int f) {
        return task.predicates[g.facts[f].first].isDerivedPredicate();
    };
    vector<int> needed;
    for (const GroundOperator &op : g.operators)
        needed.insert(needed.end(), op.negated_pre.begin(), op.negated_pre.end());
    for (const GroundConditionalEffect &ce : g.conditional_effects)
        needed.insert(needed.end(), ce.negated_condition.begin(), ce.negated_condition.end());
    needed.insert(needed.end(), g.negated_goal.begin(), g.negated_goal.end());
    sort(needed.begin(), needed.end());
    needed.erase(unique(needed.begin(), needed.end()), needed.end());

    vector<vector<int>> axioms_by_head(num_base_facts);
    for (size_t i = 0; i < g.axioms.size(); ++i)
        axioms_by_head[g.axioms[i].head].push_back(i);
    // Combinations per negated derived fact; a requirement r >= 0 is the fact
    // r itself, r < 0 is the complement of fact -r - 1.
    vector<pair<int, vector<vector<int>>>> negated_axioms;
    vector<int> needed_base;
    int num_negations_relaxed_away = 0;
    for (int d : needed) {
        if (!is_derived(d)) {
            needed_base.push_back(d);
            continue;
        }
        vector<vector<int>> bodies;
        bool possible = true;
        long long combinations = 1;
        for (int a : axioms_by_head[d]) {
            const GroundAxiom &ax = g.axioms[a];
            vector<int> literals;
            for (int f : ax.body) {
                if (is_derived(f)) {
                    possible = false;
                    break;
                }
                literals.push_back(-f - 1);
            }
            for (int f : ax.negated_body)
                literals.push_back(f);
            if (!possible)
                break;
            combinations *= max<size_t>(literals.size(), 1);
            if (combinations > 32) {
                possible = false;
                break;
            }
            bodies.push_back(literals);
        }
        if (!possible) {
            ++num_negations_relaxed_away;
            continue;
        }
        vector<vector<int>> combos(1);
        for (const vector<int> &body : bodies) {
            vector<vector<int>> next;
            for (const vector<int> &c : combos) {
                for (int lit : body) {
                    next.push_back(c);
                    next.back().push_back(lit);
                }
            }
            // An empty body is always true: not-d is unreachable.
            combos.swap(next);
        }
        for (const vector<int> &c : combos) {
            for (int lit : c) {
                if (lit < 0)
                    needed_base.push_back(-lit - 1);
            }
        }
        negated_axioms.emplace_back(d, std::move(combos));
    }
    auto add_complement = [&](int f) {
        if (complement[f] < 0) {
            complement[f] = num_facts++;
            complement_base.push_back(f);
        }
    };
    for (int f : needed_base)
        add_complement(f);
    for (const auto &entry : negated_axioms)
        add_complement(entry.first);
    auto with_complements = [&](const vector<int> &pos, const vector<int> &neg) {
        vector<int> result = pos;
        for (int f : neg) {
            if (complement[f] >= 0)
                result.push_back(complement[f]);
        }
        return result;
    };

    auto add_relaxed_op = [&](const vector<int> &pre1, const vector<int> *pre2,
                              const vector<int> &eff, int base_cost, int action) {
        if (eff.empty())
            return;
        int id = op_base_cost.size();
        op_pre_start.push_back(op_pre.size());
        op_pre.insert(op_pre.end(), pre1.begin(), pre1.end());
        if (pre2)
            op_pre.insert(op_pre.end(), pre2->begin(), pre2->end());
        // Preconditions of an operator and of its effect condition can
        // overlap.
        sort(op_pre.begin() + op_pre_start.back(), op_pre.end());
        op_pre.erase(unique(op_pre.begin() + op_pre_start.back(), op_pre.end()), op_pre.end());
        op_eff_start.push_back(op_eff.size());
        op_eff.insert(op_eff.end(), eff.begin(), eff.end());
        op_base_cost.push_back(base_cost);
        op_num_pre.push_back(op_pre.size() - op_pre_start.back());
        op_action.push_back(action);
        if (op_num_pre.back() == 0)
            no_pre_ops.push_back(id);
    };

    for (size_t o = 0; o < g.operators.size(); ++o) {
        const GroundOperator &op = g.operators[o];
        add_relaxed_op(with_complements(op.pre, op.negated_pre), nullptr,
                       with_complements(op.add, op.del), op.cost, o);
    }
    for (const GroundConditionalEffect &ce : g.conditional_effects) {
        const GroundOperator &op = g.operators[ce.op];
        vector<int> cond = with_complements(ce.condition, ce.negated_condition);
        add_relaxed_op(with_complements(op.pre, op.negated_pre), &cond,
                       with_complements(ce.add, ce.del), op.cost, ce.op);
    }
    for (const GroundAxiom &ax : g.axioms) {
        add_relaxed_op(ax.body, nullptr, vector<int>{ax.head}, 0, -1);
    }
    for (const auto &entry : negated_axioms) {
        vector<int> eff{complement[entry.first]};
        for (const vector<int> &c : entry.second) {
            vector<int> pre;
            for (int lit : c)
                pre.push_back(lit >= 0 ? lit : complement[-lit - 1]);
            add_relaxed_op(pre, nullptr, eff, 0, -1);
        }
    }
    op_pre_start.push_back(op_pre.size());
    op_eff_start.push_back(op_eff.size());
    for (const GroundOperator &op : g.operators)
        op_action_cost.push_back(op.cost);

    goal = with_complements(g.goal, g.negated_goal);
    prune_irrelevant(g, num_negations_relaxed_away > 0);

    // Precondition-of lists.
    int num_ops = op_base_cost.size();
    vector<int> count(num_facts + 1, 0);
    for (int p : op_pre)
        ++count[p + 1];
    for (int f = 0; f < num_facts; ++f)
        count[f + 1] += count[f];
    pre_of_start = count;
    pre_of.resize(op_pre.size());
    vector<int> pos(count.begin(), count.end() - 1);
    for (int r = 0; r < num_ops; ++r) {
        for (int i = op_pre_start[r]; i < op_pre_start[r + 1]; ++i)
            pre_of[pos[op_pre[i]]++] = r;
    }

    goal_unreachable = g.goal_unreachable;

    fact_state.assign(num_facts, FactState());
    relaxed_ops.resize(num_ops);
    for (int r = 0; r < num_ops; ++r) {
        RelaxedOp &o = relaxed_ops[r];
        o.base_cost = op_base_cost[r];
        o.num_pre = op_num_pre[r];
        o.eff_begin = op_eff_start[r];
        o.eff_end = op_eff_start[r + 1];
    }
    in_state.assign(num_facts, 0);
    marked_fact.assign(num_facts, 0);
    marked_action.assign(g.operators.size(), 0);
    useful_atoms.resize(task.predicates.size());
}

/*
  Backward relevance analysis: goal facts are relevant, and so are the
  preconditions of relaxed operators achieving relevant facts. Relaxed
  operators (and effects) achieving no relevant fact cannot influence the
  heuristic and are removed. A ground action is relevant if some relaxed
  operator of it achieves a relevant fact; irrelevant actions are never
  needed in a plan, unless some negated condition was relaxed away
  (negated derived atoms without negated axioms, negated axiom bodies).
*/
void GroundedHeuristic::prune_irrelevant(const GroundTask &g, bool some_negation_relaxed) {
    int num_ops = op_base_cost.size();
    vector<int> count(num_facts + 1, 0);
    for (int e : op_eff)
        ++count[e + 1];
    for (int f = 0; f < num_facts; ++f)
        count[f + 1] += count[f];
    vector<int> achievers(op_eff.size());
    vector<int> pos(count.begin(), count.end() - 1);
    for (int r = 0; r < num_ops; ++r) {
        for (int i = op_eff_start[r]; i < op_eff_start[r + 1]; ++i)
            achievers[pos[op_eff[i]]++] = r;
    }
    vector<char> relevant_fact(num_facts, 0), relevant_op(num_ops, 0);
    vector<int> stack;
    for (int f : goal) {
        if (!relevant_fact[f]) {
            relevant_fact[f] = 1;
            stack.push_back(f);
        }
    }
    while (!stack.empty()) {
        int f = stack.back();
        stack.pop_back();
        for (int k = count[f]; k < count[f + 1]; ++k) {
            int r = achievers[k];
            if (relevant_op[r])
                continue;
            relevant_op[r] = 1;
            for (int i = op_pre_start[r]; i < op_pre_start[r + 1]; ++i) {
                int p = op_pre[i];
                if (!relevant_fact[p]) {
                    relevant_fact[p] = 1;
                    stack.push_back(p);
                }
            }
        }
    }

    vector<int> new_pre_start, new_pre, new_eff_start, new_eff, new_base_cost, new_num_pre,
        new_action;
    no_pre_ops.clear();
    relevant_action.assign(g.operators.size(), 0);
    for (int r = 0; r < num_ops; ++r) {
        if (!relevant_op[r])
            continue;
        int id = new_base_cost.size();
        new_pre_start.push_back(new_pre.size());
        new_pre.insert(new_pre.end(), op_pre.begin() + op_pre_start[r],
                       op_pre.begin() + op_pre_start[r + 1]);
        new_eff_start.push_back(new_eff.size());
        for (int i = op_eff_start[r]; i < op_eff_start[r + 1]; ++i) {
            if (relevant_fact[op_eff[i]])
                new_eff.push_back(op_eff[i]);
        }
        new_base_cost.push_back(op_base_cost[r]);
        new_num_pre.push_back(op_num_pre[r]);
        new_action.push_back(op_action[r]);
        if (op_action[r] >= 0)
            relevant_action[op_action[r]] = 1;
        if (op_num_pre[r] == 0)
            no_pre_ops.push_back(id);
    }
    new_pre_start.push_back(new_pre.size());
    new_eff_start.push_back(new_eff.size());
    int num_relevant_actions = count_if(relevant_action.begin(), relevant_action.end(),
                                        [](char c) { return c; });
    cout << "Relevance analysis: " << new_base_cost.size() << " of " << num_ops
         << " relaxed operators, " << num_relevant_actions << " of " << g.operators.size()
         << " actions relevant." << endl;
    op_pre_start.swap(new_pre_start);
    op_pre.swap(new_pre);
    op_eff_start.swap(new_eff_start);
    op_eff.swap(new_eff);
    op_base_cost.swap(new_base_cost);
    op_num_pre.swap(new_num_pre);
    op_action.swap(new_action);

    bool negated_axiom_bodies = false;
    for (const GroundAxiom &ax : g.axioms) {
        if (!ax.negated_body.empty())
            negated_axiom_bodies = true;
    }
    if (some_negation_relaxed || negated_axiom_bodies)
        relevant_action.assign(g.operators.size(), 1);
}

void GroundedHeuristic::push(int fact, int cost) {
    if (cost >= int(buckets.size()))
        buckets.resize(cost + 1);
    buckets[cost].push_back(fact);
}

int GroundedHeuristic::explore() {
    // Generation stamps make untouched facts and operators implicitly reset.
    if (++generation == numeric_limits<int>::max()) {
        generation = 1;
        for (auto &f : fact_state)
            f.stamp = 0;
        for (auto &o : relaxed_ops)
            o.stamp = 0;
    }
    const int gen = generation;
    for (auto &b : buckets)
        b.clear();
    auto cost_of = [&](int f) {
        return fact_state[f].stamp == gen ? fact_state[f].cost : INF;
    };
    auto improve = [&](int f, int cost, int r) {
        FactState &fs = fact_state[f];
        if (fs.stamp != gen) {
            fs.stamp = gen;
        }
        else if (fs.cost < 0 || cost >= fs.cost) {
            return;
        }
        fs.cost = cost;
        fs.supporter = r;
        push(f, cost);
    };

    for (int f : state_facts)
        improve(f, 0, -1);
    for (int r : no_pre_ops) {
        const RelaxedOp &o = relaxed_ops[r];
        for (int i = o.eff_begin; i < o.eff_end; ++i)
            improve(op_eff[i], o.base_cost, r);
    }

    int goals_remaining = 0;
    for (int f : goal) {
        if (cost_of(f) != 0)
            ++goals_remaining;
    }
    if (goals_remaining == 0)
        return 0;

    const bool is_max = (type == MAX);
    for (size_t c = 0; c < buckets.size(); ++c) {
        for (size_t i = 0; i < buckets[c].size(); ++i) {
            int f = buckets[c][i];
            FactState &fs = fact_state[f];
            if (fs.cost != int(c))
                continue;
            // Mark as closed (a fact can be queued several times at the same
            // cost): costs are never negative, so -1 - c encodes "closed".
            fs.cost = -1 - int(c);
            for (int k = pre_of_start[f]; k < pre_of_start[f + 1]; ++k) {
                RelaxedOp &o = relaxed_ops[pre_of[k]];
                if (o.stamp != gen) {
                    o.stamp = gen;
                    o.unsatisfied = o.num_pre;
                    o.cost = 0;
                }
                if (is_max)
                    o.cost = max(o.cost, int(c));
                else
                    o.cost += c;
                if (--o.unsatisfied == 0) {
                    int cost = o.cost + o.base_cost;
                    for (int j = o.eff_begin; j < o.eff_end; ++j)
                        improve(op_eff[j], cost, pre_of[k]);
                }
            }
        }
        // Check the goals after each completed bucket: all facts of cost
        // <= c are closed then.
        bool all_closed = true;
        for (int g : goal) {
            if (cost_of(g) >= 0) {
                all_closed = false;
                break;
            }
        }
        if (all_closed)
            break;
    }

    int h = 0;
    for (int g : goal) {
        int cost = cost_of(g);
        if (cost >= 0)
            return UNSOLVABLE_STATE;
        cost = -1 - cost;
        if (is_max)
            h = max(h, cost);
        else
            h += cost;
    }
    return h;
}

void GroundedHeuristic::add_complement_facts(const vector<int> &facts, vector<int> &out) {
    if (num_facts == num_base_facts)
        return;
    // Complement facts of the base facts that are false.
    for (int f : facts)
        in_state[f] = 1;
    for (int c = num_base_facts; c < num_facts; ++c) {
        if (!in_state[complement_base[c - num_base_facts]])
            out.push_back(c);
    }
    for (int f : facts)
        in_state[f] = 0;
}

int GroundedHeuristic::compute_heuristic(const DBState &s, const Task &task) {
    vector<int> facts;
    ground_task->get_state_facts(s, facts);
    return compute_for_facts(facts, true);
}

int GroundedHeuristic::compute_for_facts(const vector<int> &facts, bool fill_useful_atoms) {
    if (fill_useful_atoms) {
        for (auto &atoms : useful_atoms)
            atoms.clear();
    }
    relaxed_plan.clear();
    useful_facts.clear();
    last_hadd = UNSOLVABLE_STATE;
    if (goal_unreachable)
        return UNSOLVABLE_STATE;

    state_facts = facts;
    add_complement_facts(facts, state_facts);
    int h = explore();
    last_hadd = h;
    if (h == UNSOLVABLE_STATE || h == 0 || type != FF)
        return h;

    // Relaxed plan extraction from the best supporters.
    for (int f : state_facts)
        in_state[f] = 1;
    int ff = 0;
    stack.clear();
    for (int g : goal) {
        if (!in_state[g])
            stack.push_back(g);
    }
    while (!stack.empty()) {
        int f = stack.back();
        stack.pop_back();
        if (marked_fact[f] || in_state[f])
            continue;
        marked_fact[f] = 1;
        marked_facts_list.push_back(f);
        int r = fact_state[f].supporter;
        int a = op_action[r];
        if (a >= 0 && !marked_action[a]) {
            marked_action[a] = 1;
            marked_actions_list.push_back(a);
            ff += op_action_cost[a];
        }
        for (int i = op_pre_start[r]; i < op_pre_start[r + 1]; ++i) {
            int p = op_pre[i];
            if (!marked_fact[p] && !in_state[p])
                stack.push_back(p);
        }
    }
    const GroundTask &g = *ground_task;
    for (int f : marked_facts_list) {
        marked_fact[f] = 0;
        if (fill_useful_atoms && f < num_base_facts)
            useful_atoms[g.facts[f].first].push_back(g.facts[f].second);
    }
    useful_facts.swap(marked_facts_list);
    marked_facts_list.clear();
    for (int a : marked_actions_list) {
        marked_action[a] = 0;
        relaxed_plan.push_back(a);
    }
    marked_actions_list.clear();
    for (int f : state_facts)
        in_state[f] = 0;
    return ff;
}
