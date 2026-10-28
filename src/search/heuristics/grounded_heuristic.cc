#include "grounded_heuristic.h"

#include "../action_schema.h"
#include "../grounding/ground_task.h"
#include "../task.h"
#include "../states/state.h"
#include "../utils/system.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <cstdlib>
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
    for (const GroundTask::RelaxationOperator &op : g.relaxation_operators) {
        for (int f : op.pre) {
            if (f < 0)
                needed.push_back(-1 - f);
        }
    }
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

    first_aux_fact = num_facts;
    vector<char> decomposed(task.get_action_schemas().size(), 0);
    if (!getenv("PWL_NO_DECOMPOSE"))
        add_decomposed_operators(task, g, decomposed);
    for (size_t o = 0; o < g.operators.size(); ++o) {
        const GroundOperator &op = g.operators[o];
        if (decomposed[op.schema])
            continue;
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
    if (g.operators_omitted)
        add_relaxation_operators(task, g);
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

    // Achiever lists.
    vector<int> ach_count(num_facts + 1, 0);
    for (int e : op_eff)
        ++ach_count[e + 1];
    for (int f = 0; f < num_facts; ++f)
        ach_count[f + 1] += ach_count[f];
    ach_start = ach_count;
    ach.resize(op_eff.size());
    vector<int> ach_pos(ach_count.begin(), ach_count.end() - 1);
    for (int r = 0; r < num_ops; ++r) {
        for (int i = op_eff_start[r]; i < op_eff_start[r + 1]; ++i)
            ach[ach_pos[op_eff[i]]++] = r;
    }
    if (getenv("PWL_FF_REUSE"))
        reuse_slack = atoi(getenv("PWL_FF_REUSE"));

    goal_unreachable = g.goal_unreachable;

    fact_state.assign(num_facts, FactState());
    relaxed_ops.resize(num_ops);
    for (int r = 0; r < num_ops; ++r)
        relaxed_ops[r].num_pre = op_num_pre[r];
    in_state.assign(num_facts, 0);
    marked_fact.assign(num_facts, 0);
    marked_action.assign(g.operators.size(), 0);
    useful_atoms.resize(task.predicates.size());
}

int GroundedHeuristic::add_relaxed_op(const vector<int> &pre1, const vector<int> *pre2,
                                      const vector<int> &eff, int base_cost, int action) {
    if (eff.empty())
        return -1;
    int id = op_base_cost.size();
    op_pre_start.push_back(op_pre.size());
    op_pre.insert(op_pre.end(), pre1.begin(), pre1.end());
    if (pre2)
        op_pre.insert(op_pre.end(), pre2->begin(), pre2->end());
    // Preconditions of an operator and of its effect condition can overlap.
    sort(op_pre.begin() + op_pre_start.back(), op_pre.end());
    op_pre.erase(unique(op_pre.begin() + op_pre_start.back(), op_pre.end()), op_pre.end());
    op_eff_start.push_back(op_eff.size());
    op_eff.insert(op_eff.end(), eff.begin(), eff.end());
    op_base_cost.push_back(base_cost);
    op_num_pre.push_back(op_pre.size() - op_pre_start.back());
    op_action.push_back(action);
    op_rule.push_back(-1);
    op_values_start.push_back(rule_values.size());
    if (op_num_pre.back() == 0)
        no_pre_ops.push_back(id);
    return id;
}

/*
  Join decomposition of relaxed operators. The relaxed operators of a large
  action schema (without conditional effects) are replaced by the ground
  instances of a decomposed rule program, built by variable elimination on
  the schema's precondition: for each group of effect literals with the
  same variables H, every variable not in H is eliminated in turn by a rule
  joining the precondition literals (and auxiliary atoms) containing it and
  deriving an auxiliary atom over their other variables; a final rule
  derives the effects from the remaining atoms. Static atoms and
  (in)equalities take part in the elimination (they couple variables) but
  have no facts.

  The instances of a rule are the projections of the schema's ground
  operators onto the rule's variables, so every combination of instances
  corresponds to a ground operator, and h-add and h-max are the same as on
  the ground relaxed operators (min and sum distribute over the
  elimination). The ground operator of a final instance in a relaxed plan is
  found by following the supporters of its auxiliary preconditions.
*/
namespace {
vector<int> variables_of(const Atom &atom) {
    vector<int> vars;
    for (const Argument &arg : atom.get_arguments()) {
        if (!arg.is_constant())
            vars.push_back(arg.get_index());
    }
    sort(vars.begin(), vars.end());
    vars.erase(unique(vars.begin(), vars.end()), vars.end());
    return vars;
}

GroundAtom instantiate(const Atom &atom, const vector<int> &values) {
    GroundAtom ga;
    for (const Argument &arg : atom.get_arguments())
        ga.push_back(arg.is_constant() ? arg.get_index() : values[arg.get_index()]);
    return ga;
}

GroundAtom project(const vector<int> &values, const vector<int> &vars) {
    GroundAtom key;
    for (int v : vars)
        key.push_back(values[v]);
    return key;
}
}  // namespace

void GroundedHeuristic::add_decomposed_operators(const Task &task, const GroundTask &g,
                                                 vector<char> &decomposed) {
    const auto &schemas = task.get_action_schemas();
    vector<vector<int>> ops_of(schemas.size());
    for (size_t o = 0; o < g.operators.size(); ++o)
        ops_of[g.operators[o].schema].push_back(o);
    auto is_static = [&](int p) { return task.predicates[p].isStaticPredicate(); };
    const size_t min_operators = getenv("PWL_DECOMPOSE_MIN")
        ? atoi(getenv("PWL_DECOMPOSE_MIN")) : 1000;
    size_t total_ground = 0, total_decomposed = 0;

    for (size_t a = 0; a < schemas.size(); ++a) {
        const ActionSchema &schema = schemas[a];
        const vector<int> &ops = ops_of[a];
        if (schema.has_conditional_effects() || ops.size() < min_operators)
            continue;
        const int num_vars = g.operators[ops[0]].instantiation.size();

        // Atoms of the hypergraph: precondition literals (kind 0), static
        // atoms and (in)equalities (kind 1), auxiliary atoms (kind 2).
        struct HAtom {
            vector<int> vars;
            int kind;
            int ref;
        };
        vector<const Atom *> literals;
        vector<HAtom> base_atoms;
        for (const Atom &pre : schema.get_precondition()) {
            if (is_static(pre.get_predicate_symbol_idx())) {
                base_atoms.push_back({variables_of(pre), 1, -1});
            }
            else {
                base_atoms.push_back({variables_of(pre), 0, int(literals.size())});
                literals.push_back(&pre);
            }
        }
        for (const Atom &eq : schema.get_static_precondition())
            base_atoms.push_back({variables_of(eq), 1, -1});
        // Nullary preconditions and effects (the same for all operators).
        vector<int> nullary_pre, nullary_eff;
        const auto &pos_pre = schema.get_positive_nullary_precond();
        const auto &neg_pre = schema.get_negative_nullary_precond();
        for (size_t p = 0; p < pos_pre.size(); ++p) {
            if (pos_pre[p] && !is_static(p)) {
                int f = g.get_fact(p, GroundAtom());
                if (f >= 0)
                    nullary_pre.push_back(f);
            }
        }
        for (size_t p = 0; p < neg_pre.size(); ++p) {
            if (neg_pre[p] && !is_static(p)) {
                int f = g.get_fact(p, GroundAtom());
                if (f >= 0 && complement[f] >= 0)
                    nullary_pre.push_back(complement[f]);
            }
        }
        const auto &pos_eff = schema.get_positive_nullary_effects();
        const auto &neg_eff = schema.get_negative_nullary_effects();
        for (size_t p = 0; p < pos_eff.size(); ++p) {
            if (pos_eff[p]) {
                int f = g.get_fact(p, GroundAtom());
                if (f >= 0)
                    nullary_eff.push_back(f);
            }
        }
        for (size_t p = 0; p < neg_eff.size(); ++p) {
            if (neg_eff[p]) {
                int f = g.get_fact(p, GroundAtom());
                if (f >= 0 && complement[f] >= 0)
                    nullary_eff.push_back(complement[f]);
            }
        }
        // Effect groups (effect literals with the same variables).
        map<vector<int>, vector<const Atom *>> groups;
        for (const Atom &eff : schema.get_effects())
            groups[variables_of(eff)].push_back(&eff);
        if (!nullary_eff.empty())
            groups[vector<int>()];

        // Number of values of each variable (for the elimination order).
        vector<double> domain(num_vars, 1);
        for (int v = 0; v < num_vars; ++v) {
            vector<int> values;
            for (int o : ops)
                values.push_back(g.operators[o].instantiation[v]);
            sort(values.begin(), values.end());
            domain[v] = unique(values.begin(), values.end()) - values.begin();
        }

        // Variables with a single value are constants.
        for (HAtom &h : base_atoms) {
            h.vars.erase(remove_if(h.vars.begin(), h.vars.end(),
                                   [&](int v) { return domain[v] == 1; }),
                         h.vars.end());
        }
        // True if the projection of the operators onto vars is the product
        // of the projections onto the single variables.
        auto is_product = [&](const vector<int> &vars) {
            phmap::flat_hash_set<GroundAtom, TupleHash> tuples;
            double product = 1;
            for (int v : vars)
                product *= domain[v];
            for (int o : ops) {
                tuples.insert(project(g.operators[o].instantiation, vars));
                if (tuples.size() > product)
                    return false;
            }
            return double(tuples.size()) == product;
        };

        // Rules: auxiliary rules first (in creation order), then the final
        // rule of each group.
        struct LRule {
            vector<int> vars;
            vector<int> body_literals;
            vector<int> body_aux;
            vector<int> head_vars;
            bool final = false;
            vector<const Atom *> effects;
            bool nullary_effects = false;
            phmap::flat_hash_map<GroundAtom, int, TupleHash> instances;
            phmap::flat_hash_map<GroundAtom, int, TupleHash> head_facts;
        };
        vector<LRule> rules;
        map<pair<vector<pair<int, int>>, vector<int>>, int> memo;
        for (const auto &group : groups) {
            const vector<int> &head = group.first;
            vector<char> in_head(num_vars, 0);
            for (int v : head)
                in_head[v] = 1;
            vector<HAtom> atoms = base_atoms;
            while (true) {
                int best = -1;
                bool best_static = false;
                double best_size = 0;
                for (int v = 0; v < num_vars; ++v) {
                    if (in_head[v])
                        continue;
                    vector<char> in_u(num_vars, 0);
                    bool occurs = false, has_fact = false;
                    for (const HAtom &h : atoms) {
                        if (!binary_search(h.vars.begin(), h.vars.end(), v))
                            continue;
                        occurs = true;
                        has_fact = has_fact || h.kind != 1;
                        for (int u : h.vars)
                            in_u[u] = 1;
                    }
                    if (!occurs)
                        continue;
                    double size = 1;
                    for (int u = 0; u < num_vars; ++u) {
                        if (in_u[u])
                            size *= domain[u];
                    }
                    // Eliminate variables in static atoms only first.
                    bool is_static_only = !has_fact;
                    if (best < 0 || (is_static_only && !best_static) ||
                        (is_static_only == best_static && size < best_size)) {
                        best = v;
                        best_static = is_static_only;
                        best_size = size;
                    }
                }
                if (best < 0)
                    break;
                vector<HAtom> rest, body;
                for (HAtom &h : atoms) {
                    if (binary_search(h.vars.begin(), h.vars.end(), best))
                        body.push_back(std::move(h));
                    else
                        rest.push_back(std::move(h));
                }
                vector<int> u;
                for (const HAtom &h : body)
                    u.insert(u.end(), h.vars.begin(), h.vars.end());
                sort(u.begin(), u.end());
                u.erase(unique(u.begin(), u.end()), u.end());
                vector<int> head_vars;
                for (int x : u) {
                    if (x != best)
                        head_vars.push_back(x);
                }
                if (best_static) {
                    // A static constraint on the remaining variables, unless
                    // they are independent.
                    if (head_vars.size() > 1 && !is_product(head_vars))
                        rest.push_back({head_vars, 1, -1});
                }
                else {
                    vector<pair<int, int>> signature;
                    for (const HAtom &h : body) {
                        if (h.kind != 1)
                            signature.emplace_back(h.kind, h.ref);
                    }
                    sort(signature.begin(), signature.end());
                    signature.emplace_back(-1, best);
                    auto key = make_pair(signature, u);
                    auto it = memo.find(key);
                    int rule_id;
                    if (it != memo.end()) {
                        rule_id = it->second;
                    }
                    else {
                        rule_id = rules.size();
                        rules.emplace_back();
                        LRule &r = rules.back();
                        r.vars = u;
                        r.head_vars = head_vars;
                        for (const HAtom &h : body) {
                            if (h.kind == 0)
                                r.body_literals.push_back(h.ref);
                            else if (h.kind == 2)
                                r.body_aux.push_back(h.ref);
                        }
                        memo.emplace(key, rule_id);
                    }
                    rest.push_back({head_vars, 2, rule_id});
                }
                atoms.swap(rest);
            }
            rules.emplace_back();
            LRule &r = rules.back();
            r.final = true;
            r.vars = head;
            for (const HAtom &h : atoms) {
                r.vars.insert(r.vars.end(), h.vars.begin(), h.vars.end());
                if (h.kind == 0)
                    r.body_literals.push_back(h.ref);
                else if (h.kind == 2)
                    r.body_aux.push_back(h.ref);
            }
            sort(r.vars.begin(), r.vars.end());
            r.vars.erase(unique(r.vars.begin(), r.vars.end()), r.vars.end());
            r.effects = group.second;
            r.nullary_effects = head.empty() && !nullary_eff.empty();
        }

        // Ground the rules on the projections of the ground operators.
        struct Instance {
            int rule;
            vector<int> pre;
            vector<int> eff;
            GroundAtom values;
        };
        vector<Instance> instances;
        int num_aux = 0;
        size_t ground_size = 0, decomposed_size = 0;
        vector<int> literal_fact(literals.size());
        bool too_large = false;
        for (int o : ops) {
            const GroundOperator &op = g.operators[o];
            ground_size += op.pre.size() + op.negated_pre.size() + op.add.size() + op.del.size();
        }
        for (int o : ops) {
            const GroundOperator &op = g.operators[o];
            const vector<int> &values = op.instantiation;
            for (size_t i = 0; i < literals.size(); ++i) {
                const Atom &lit = *literals[i];
                int f = g.get_fact(lit.get_predicate_symbol_idx(), instantiate(lit, values));
                if (lit.is_negated())
                    f = f >= 0 ? complement[f] : -1;
                literal_fact[i] = f;
            }
            for (size_t ri = 0; ri < rules.size(); ++ri) {
                LRule &r = rules[ri];
                auto inserted = r.instances.emplace(project(values, r.vars), 0);
                if (!inserted.second)
                    continue;
                Instance inst;
                inst.rule = ri;
                inst.values = inserted.first->first;
                for (int i : r.body_literals) {
                    if (literal_fact[i] >= 0)
                        inst.pre.push_back(literal_fact[i]);
                }
                for (int c : r.body_aux) {
                    auto it = rules[c].head_facts.find(project(values, rules[c].head_vars));
                    assert(it != rules[c].head_facts.end());
                    inst.pre.push_back(it->second);
                }
                if (!r.final) {
                    auto h = r.head_facts.emplace(project(values, r.head_vars),
                                                  num_facts + num_aux);
                    if (h.second)
                        ++num_aux;
                    inst.eff.push_back(h.first->second);
                }
                else {
                    inst.pre.insert(inst.pre.end(), nullary_pre.begin(), nullary_pre.end());
                    for (const Atom *e : r.effects) {
                        int f = g.get_fact(e->get_predicate_symbol_idx(), instantiate(*e, values));
                        if (e->is_negated())
                            f = f >= 0 ? complement[f] : -1;
                        if (f >= 0)
                            inst.eff.push_back(f);
                    }
                    if (r.nullary_effects)
                        inst.eff.insert(inst.eff.end(), nullary_eff.begin(), nullary_eff.end());
                }
                decomposed_size += inst.pre.size() + inst.eff.size();
                instances.push_back(std::move(inst));
            }
            if (decomposed_size * 10 > ground_size * 8) {
                too_large = true;
                break;
            }
        }
        total_ground += ground_size;
        if (too_large) {
            total_decomposed += ground_size;
            continue;
        }
        total_decomposed += decomposed_size;

        // Commit: auxiliary facts, operator indices, and relaxed operators.
        decomposed[a] = 1;
        decomposed_operators.insert(decomposed_operators.end(), ops.begin(), ops.end());
        num_facts += num_aux;
        vector<int> rule_id(rules.size());
        for (size_t ri = 0; ri < rules.size(); ++ri) {
            DecomposedRule dr;
            dr.vars = rules[ri].vars;
            dr.op_index = -1;
            if (rules[ri].final) {
                // Key: the head and precondition literal variables.
                vector<int> key = rules[ri].vars;
                for (const Atom *lit : literals) {
                    vector<int> v = variables_of(*lit);
                    key.insert(key.end(), v.begin(), v.end());
                }
                sort(key.begin(), key.end());
                key.erase(unique(key.begin(), key.end()), key.end());
                // Single-valued variables are no rule variables.
                key.erase(remove_if(key.begin(), key.end(),
                                    [&](int v) { return domain[v] == 1; }),
                          key.end());
                for (size_t k = 0; k < operator_indices.size(); ++k) {
                    if (operator_indices[k].schema == int(a) && operator_indices[k].vars == key)
                        dr.op_index = k;
                }
                if (dr.op_index < 0) {
                    OperatorIndex index;
                    index.schema = a;
                    index.vars = key;
                    index.ops = ops;
                    auto less = [&](int x, int y) {
                        const auto &vx = g.operators[x].instantiation;
                        const auto &vy = g.operators[y].instantiation;
                        for (int v : key) {
                            if (vx[v] != vy[v])
                                return vx[v] < vy[v];
                        }
                        return false;
                    };
                    stable_sort(index.ops.begin(), index.ops.end(), less);
                    dr.op_index = operator_indices.size();
                    operator_indices.push_back(std::move(index));
                }
            }
            rule_id[ri] = decomposed_rules.size();
            decomposed_rules.push_back(std::move(dr));
        }
        // Skip duplicate auxiliary instances (same precondition and head).
        set<pair<vector<int>, int>> seen;
        for (Instance &inst : instances) {
            sort(inst.pre.begin(), inst.pre.end());
            inst.pre.erase(unique(inst.pre.begin(), inst.pre.end()), inst.pre.end());
            const LRule &r = rules[inst.rule];
            if (!r.final && !seen.emplace(inst.pre, inst.eff[0]).second)
                continue;
            int id = add_relaxed_op(inst.pre, nullptr, inst.eff,
                                    r.final ? schema.get_cost() : 0, r.final ? -2 : -1);
            if (id < 0)
                continue;
            op_rule[id] = rule_id[inst.rule];
            op_values_start[id] = rule_values.size();
            rule_values.insert(rule_values.end(), inst.values.begin(), inst.values.end());
        }
        cout << "Decomposed schema " << schema.get_name() << ": " << ops.size()
             << " operators, " << instances.size() << " rule instances, " << num_aux
             << " auxiliary facts." << endl;
    }
    if (total_ground > 0)
        cout << "Decomposition: relaxed operator size " << total_decomposed << " instead of "
             << total_ground << "." << endl;
}

void GroundedHeuristic::get_ground_relaxed_operator(int o, vector<int> &pre,
                                                    vector<int> &eff) const {
    const GroundOperator &op = ground_task->operators[o];
    pre = op.pre;
    for (int f : op.negated_pre) {
        if (complement[f] >= 0)
            pre.push_back(complement[f]);
    }
    eff = op.add;
    for (int f : op.del) {
        if (complement[f] >= 0)
            eff.push_back(complement[f]);
    }
}

void GroundedHeuristic::add_relaxation_operators(const Task &task, const GroundTask &g) {
    const int aux_base = num_facts;
    num_facts += g.num_relaxation_aux;
    const int rule_base = decomposed_rules.size();
    for (const GroundTask::RelaxationRule &rule : g.relaxation_rules) {
        DecomposedRule dr;
        dr.vars = rule.vars;
        dr.op_index = -1;
        dr.schema = rule.schema;
        dr.num_params = task.get_action_schema_by_index(rule.schema).get_parameters().size();
        decomposed_rules.push_back(std::move(dr));
    }
    const int num_ground_facts = g.num_facts();
    auto map_fact = [&](int f) {
        if (f >= num_ground_facts)
            return aux_base + (f - num_ground_facts);
        if (f >= 0)
            return f;
        return complement[-1 - f];
    };
    vector<int> pre, eff;
    for (const GroundTask::RelaxationOperator &op : g.relaxation_operators) {
        pre.clear();
        eff.clear();
        for (int f : op.pre) {
            int x = map_fact(f);
            if (x >= 0)
                pre.push_back(x);
        }
        for (int f : op.eff) {
            int x = map_fact(f);
            if (x >= 0)
                eff.push_back(x);
        }
        const GroundTask::RelaxationRule &rule = g.relaxation_rules[op.rule];
        int id = add_relaxed_op(pre, nullptr, eff, rule.cost, rule.final ? -3 : -1);
        if (id < 0)
            continue;
        op_rule[id] = rule_base + op.rule;
        op_values_start[id] = rule_values.size();
        rule_values.insert(rule_values.end(), op.values.begin(), op.values.end());
    }
    cout << "Relaxed program operators: " << g.relaxation_operators.size() << "." << endl;
}

int GroundedHeuristic::extract_lifted_action(int r) {
    const DecomposedRule &final_rule = decomposed_rules[op_rule[r]];
    vector<int> action(final_rule.num_params + 1, -1);
    action[0] = final_rule.schema;
    reconstruct_stack.clear();
    reconstruct_stack.push_back(r);
    while (!reconstruct_stack.empty()) {
        int x = reconstruct_stack.back();
        reconstruct_stack.pop_back();
        const DecomposedRule &rule = decomposed_rules[op_rule[x]];
        const int *values = rule_values.data() + op_values_start[x];
        for (size_t i = 0; i < rule.vars.size(); ++i) {
            if (rule.vars[i] < final_rule.num_params)
                action[rule.vars[i] + 1] = values[i];
        }
        for (int i = op_pre_start[x]; i < op_pre_start[x + 1]; ++i) {
            int p = op_pre[i];
            if (p >= first_aux_fact)
                reconstruct_stack.push_back(fact_state[p].supporter);
            else if (!marked_fact[p] && !in_state[p])
                stack.push_back(p);
        }
    }
    for (const vector<int> &a : lifted_plan_actions) {
        if (a == action)
            return 0;
    }
    lifted_plan_actions.push_back(std::move(action));
    return op_base_cost[r];
}

int GroundedHeuristic::reconstruct_operator(int r) {
    const DecomposedRule &final_rule = decomposed_rules[op_rule[r]];
    const OperatorIndex &index = operator_indices[final_rule.op_index];
    const auto &ops = ground_task->operators;
    if (binding.size() < ops[index.ops[0]].instantiation.size())
        binding.resize(ops[index.ops[0]].instantiation.size());
    reconstruct_stack.clear();
    reconstruct_stack.push_back(r);
    while (!reconstruct_stack.empty()) {
        int x = reconstruct_stack.back();
        reconstruct_stack.pop_back();
        const DecomposedRule &rule = decomposed_rules[op_rule[x]];
        const int *values = rule_values.data() + op_values_start[x];
        for (size_t i = 0; i < rule.vars.size(); ++i)
            binding[rule.vars[i]] = values[i];
        for (int i = op_pre_start[x]; i < op_pre_start[x + 1]; ++i) {
            int p = op_pre[i];
            if (p >= first_aux_fact)
                reconstruct_stack.push_back(fact_state[p].supporter);
        }
    }
    auto cmp = [&](int o, int) {
        const auto &v = ops[o].instantiation;
        for (int x : index.vars) {
            if (v[x] != binding[x])
                return v[x] < binding[x];
        }
        return false;
    };
    auto it = lower_bound(index.ops.begin(), index.ops.end(), 0, cmp);
    if (it == index.ops.end())
        return -1;
    const auto &v = ops[*it].instantiation;
    for (int x : index.vars) {
        if (v[x] != binding[x])
            return -1;
    }
    return *it;
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
        new_action, new_rule, new_values_start;
    no_pre_ops.clear();
    relevant_action.assign(g.operators.size(), 0);
    bool some_decomposed = false;
    // Number the relevant operators by their smallest precondition, so that
    // the operators sharing a precondition are close in memory (the
    // exploration visits them together).
    vector<int> order;
    for (int r = 0; r < num_ops; ++r) {
        if (relevant_op[r])
            order.push_back(r);
    }
    if (!getenv("PWL_NO_OP_REORDER")) {
        auto first_pre = [&](int r) {
            return op_pre_start[r] < op_pre_start[r + 1] ? op_pre[op_pre_start[r]] : -1;
        };
        if (!getenv("PWL_OP_ORDER_FIRST_PRE")) {
            // Layer (unit-cost h-max) at which each operator becomes
            // applicable in the relaxed exploration from the initial state.
            vector<int> fact_level(num_facts, numeric_limits<int>::max());
            vector<int> op_level(num_ops, numeric_limits<int>::max());
            vector<int> unsat(op_num_pre.begin(), op_num_pre.end());
            vector<int> init = g.initial_facts;
            vector<char> init_true(num_base_facts, 0);
            for (int f : g.initial_facts)
                init_true[f] = 1;
            for (int c = num_base_facts; c < first_aux_fact; ++c) {
                if (!init_true[complement_base[c - num_base_facts]])
                    init.push_back(c);
            }
            // Precondition-of lists of the unpruned operators.
            vector<vector<int>> pre_of_tmp(num_facts);
            for (int r = 0; r < num_ops; ++r)
                for (int i = op_pre_start[r]; i < op_pre_start[r + 1]; ++i)
                    pre_of_tmp[op_pre[i]].push_back(r);
            vector<int> layer = init, next_layer;
            for (int f : init)
                fact_level[f] = 0;
            for (int r = 0; r < num_ops; ++r) {
                if (unsat[r] == 0)
                    op_level[r] = 0;
            }
            int level = 0;
            while (!layer.empty()) {
                next_layer.clear();
                for (int f : layer) {
                    for (int r : pre_of_tmp[f]) {
                        if (--unsat[r] == 0) {
                            op_level[r] = level;
                            for (int i = op_eff_start[r]; i < op_eff_start[r + 1]; ++i) {
                                int e = op_eff[i];
                                if (fact_level[e] == numeric_limits<int>::max()) {
                                    fact_level[e] = level + 1;
                                    next_layer.push_back(e);
                                }
                            }
                        }
                    }
                }
                layer.swap(next_layer);
                ++level;
            }
            stable_sort(order.begin(), order.end(), [&](int a, int b) {
                return make_pair(op_level[a], first_pre(a)) < make_pair(op_level[b], first_pre(b));
            });
        }
        else {
            stable_sort(order.begin(), order.end(),
                        [&](int a, int b) { return first_pre(a) < first_pre(b); });
        }
    }
    for (int r : order) {
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
        new_rule.push_back(op_rule[r]);
        new_values_start.push_back(op_values_start[r]);
        if (op_action[r] >= 0)
            relevant_action[op_action[r]] = 1;
        else if (op_action[r] == -2)
            some_decomposed = true;
        if (op_num_pre[r] == 0)
            no_pre_ops.push_back(id);
    }
    new_pre_start.push_back(new_pre.size());
    new_eff_start.push_back(new_eff.size());
    if (some_decomposed) {
        // Ground actions of decomposed schemas: relevant if they achieve a
        // relevant fact.
        for (size_t o = 0; o < g.operators.size(); ++o) {
            const GroundOperator &op = g.operators[o];
            if (relevant_action[o])
                continue;
            for (int f : op.add) {
                if (relevant_fact[f])
                    relevant_action[o] = 1;
            }
            for (int f : op.del) {
                if (complement[f] >= 0 && relevant_fact[complement[f]])
                    relevant_action[o] = 1;
            }
        }
    }
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
    op_rule.swap(new_rule);
    op_values_start.swap(new_values_start);

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
        for (int i = op_eff_start[r]; i < op_eff_start[r + 1]; ++i)
            improve(op_eff[i], op_base_cost[r], r);
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
                int r = pre_of[k];
                RelaxedOp &o = relaxed_ops[r];
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
                    int cost = o.cost + op_base_cost[r];
                    for (int j = op_eff_start[r]; j < op_eff_start[r + 1]; ++j)
                        improve(op_eff[j], cost, r);
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
    if (first_aux_fact == num_base_facts)
        return;
    // Complement facts of the base facts that are false.
    for (int f : facts)
        in_state[f] = 1;
    for (int c = num_base_facts; c < first_aux_fact; ++c) {
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
    relaxed_plan_levels.clear();
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
    lifted_plan_actions.clear();
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
        if (reuse_slack >= 0 && a >= 0 && !marked_action[a]) {
            // Prefer an achiever of an action already in the relaxed plan
            // if it is not much more expensive.
            int c = -1 - fact_state[f].cost;
            for (int k = ach_start[f]; k < ach_start[f + 1]; ++k) {
                int r2 = ach[k];
                int a2 = op_action[r2];
                if (a2 < 0 || !marked_action[a2])
                    continue;
                const RelaxedOp &o = relaxed_ops[r2];
                if (o.stamp == generation && o.unsatisfied == 0 &&
                    o.cost + op_base_cost[r2] <= c + reuse_slack) {
                    r = r2;
                    a = a2;
                    break;
                }
            }
        }
        if (a == -3) {
            // Relaxed program operator (no ground operators).
            ff += extract_lifted_action(r);
            continue;
        }
        if (a == -2) {
            // Decomposed operator: the ground operator and its precondition.
            a = reconstruct_operator(r);
            if (a < 0) {
                cerr << "Could not reconstruct a decomposed operator." << endl;
                utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
            }
            const GroundOperator &op = ground_task->operators[a];
            for (int p : op.pre) {
                if (!marked_fact[p] && !in_state[p])
                    stack.push_back(p);
            }
            for (int n : op.negated_pre) {
                int p = complement[n];
                if (p >= 0 && !marked_fact[p] && !in_state[p])
                    stack.push_back(p);
            }
        }
        else {
            for (int i = op_pre_start[r]; i < op_pre_start[r + 1]; ++i) {
                int p = op_pre[i];
                if (!marked_fact[p] && !in_state[p])
                    stack.push_back(p);
            }
        }
        if (a >= 0 && !marked_action[a]) {
            marked_action[a] = 1;
            marked_actions_list.push_back(a);
            // Cost (h-add) at which the action's relaxed operator fired.
            relaxed_plan_levels.push_back(-1 - fact_state[f].cost);
            ff += op_action_cost[a];
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
