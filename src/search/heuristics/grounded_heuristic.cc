#include "grounded_heuristic.h"

#include "../grounding/ground_task.h"
#include "../states/state.h"

#include <algorithm>
#include <limits>

using namespace std;

static const int INF = numeric_limits<int>::max() / 2;

GroundedHeuristic::GroundedHeuristic(const Task &task, shared_ptr<GroundTask> gt, Type type)
    : type(type), ground_task(std::move(gt)) {
    const GroundTask &g = *ground_task;
    num_base_facts = g.num_facts();
    complement.assign(num_base_facts, -1);
    num_facts = num_base_facts;
    // Derived atoms are never deleted by operators, so their complements
    // could only be "achieved" by the state; negated derived atoms are
    // relaxed away instead (like the Datalog heuristics do).
    auto need_complement = [&](const vector<int> &facts) {
        for (int f : facts) {
            if (task.predicates[g.facts[f].first].isDerivedPredicate())
                continue;
            if (complement[f] < 0) {
                complement[f] = num_facts++;
                complement_base.push_back(f);
            }
        }
    };
    for (const GroundOperator &op : g.operators)
        need_complement(op.negated_pre);
    for (const GroundConditionalEffect &ce : g.conditional_effects)
        need_complement(ce.negated_condition);
    need_complement(g.negated_goal);
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
    op_pre_start.push_back(op_pre.size());
    op_eff_start.push_back(op_eff.size());
    for (const GroundOperator &op : g.operators)
        op_action_cost.push_back(op.cost);

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

    goal = with_complements(g.goal, g.negated_goal);
    goal_unreachable = g.goal_unreachable;

    fact_cost.resize(num_facts);
    supporter.resize(num_facts);
    op_unsatisfied.resize(num_ops);
    op_cost.resize(num_ops);
    in_state.assign(num_facts, 0);
    marked_fact.assign(num_facts, 0);
    marked_action.assign(g.operators.size(), 0);
    useful_atoms.resize(task.predicates.size());
}

void GroundedHeuristic::push(int fact, int cost) {
    if (cost >= int(buckets.size()))
        buckets.resize(cost + 1);
    buckets[cost].push_back(fact);
}

int GroundedHeuristic::explore() {
    fill(fact_cost.begin(), fact_cost.end(), INF);
    copy(op_num_pre.begin(), op_num_pre.end(), op_unsatisfied.begin());
    fill(op_cost.begin(), op_cost.end(), 0);
    for (auto &b : buckets)
        b.clear();

    for (int f : state_facts) {
        if (fact_cost[f] != 0) {
            fact_cost[f] = 0;
            supporter[f] = -1;
            push(f, 0);
        }
    }
    for (int r : no_pre_ops) {
        int cost = op_base_cost[r];
        for (int i = op_eff_start[r]; i < op_eff_start[r + 1]; ++i) {
            int e = op_eff[i];
            if (cost < fact_cost[e]) {
                fact_cost[e] = cost;
                supporter[e] = r;
                push(e, cost);
            }
        }
    }

    int goals_remaining = 0;
    for (int f : goal) {
        if (fact_cost[f] != 0)
            ++goals_remaining;
    }
    if (goals_remaining == 0)
        return 0;

    const bool is_max = (type == MAX);
    for (size_t c = 0; c < buckets.size(); ++c) {
        for (size_t i = 0; i < buckets[c].size(); ++i) {
            int f = buckets[c][i];
            if (fact_cost[f] != int(c))
                continue;
            // Mark as closed (a fact can be queued several times at the same
            // cost): costs are never negative, so -1 - c encodes "closed".
            fact_cost[f] = -1 - int(c);
            for (int k = pre_of_start[f]; k < pre_of_start[f + 1]; ++k) {
                int r = pre_of[k];
                if (is_max)
                    op_cost[r] = max(op_cost[r], int(c));
                else
                    op_cost[r] += c;
                if (--op_unsatisfied[r] == 0) {
                    int cost = op_cost[r] + op_base_cost[r];
                    for (int j = op_eff_start[r]; j < op_eff_start[r + 1]; ++j) {
                        int e = op_eff[j];
                        if (fact_cost[e] >= 0 && cost < fact_cost[e]) {
                            fact_cost[e] = cost;
                            supporter[e] = r;
                            push(e, cost);
                        }
                    }
                }
            }
        }
        // Check the goals after each completed bucket: all facts of cost
        // <= c are closed then.
        bool all_closed = true;
        for (int g : goal) {
            if (fact_cost[g] >= 0) {
                all_closed = false;
                break;
            }
        }
        if (all_closed)
            break;
    }

    int h = 0;
    for (int g : goal) {
        if (fact_cost[g] >= 0)
            return UNSOLVABLE_STATE;
        int cost = -1 - fact_cost[g];
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
    if (goal_unreachable)
        return UNSOLVABLE_STATE;

    state_facts = facts;
    add_complement_facts(facts, state_facts);
    int h = explore();
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
        int r = supporter[f];
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
