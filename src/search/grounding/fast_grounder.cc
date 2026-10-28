#include "fast_grounder.h"

#include "../action_schema.h"
#include "../axiom.h"
#include "../task.h"

#include "../parallel_hashmap/phmap.h"

#include <algorithm>
#include <cassert>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <tuple>

using namespace std;

namespace {
struct Relation {
    int arity = 0;
    vector<GroundAtom> tuples;
    //! Time at which each tuple was added (a global counter).
    vector<int> times;
    //! All atoms, with the time at which they were added.
    phmap::flat_hash_map<GroundAtom, int, TupleHash> members;

    struct Index {
        vector<int> positions;
        phmap::flat_hash_map<GroundAtom, vector<int>, TupleHash> map;
    };
    vector<Index> indices;

    int get_index(const vector<int> &positions) {
        for (size_t i = 0; i < indices.size(); ++i) {
            if (indices[i].positions == positions)
                return i;
        }
        Index index;
        index.positions = positions;
        for (size_t t = 0; t < tuples.size(); ++t)
            index.map[project(tuples[t], positions)].push_back(t);
        indices.push_back(std::move(index));
        return indices.size() - 1;
    }

    static GroundAtom project(const GroundAtom &t, const vector<int> &positions) {
        GroundAtom key;
        for (int p : positions)
            key.push_back(t[p]);
        return key;
    }

    //! A sink relation is never joined (no triggers, no indices): its atoms
    //! are only kept in `members`.
    bool sink = false;

    bool add(const GroundAtom &t, int time) {
        if (!members.emplace(t, time).second)
            return false;
        if (sink)
            return true;
        int id = tuples.size();
        tuples.push_back(t);
        times.push_back(time);
        for (Index &index : indices)
            index.map[project(t, index.positions)].push_back(id);
        return true;
    }
};

// Argument of an atom in a rule: a variable (value = variable index) or a
// constant (value = object index).
struct RuleArg {
    bool is_var;
    int value;
};

struct RuleAtom {
    int relation;
    vector<RuleArg> args;
};

// Filter: (in)equality between two arguments, or a negated static atom. A
// NEGATED_FLUENT filter is a negated fluent atom: it always holds (as in the
// relaxed reachability analysis) but is placed like the other filters when
// rules are split (relaxed program of the heuristic).
struct Filter {
    enum Type {EQUAL, NOT_EQUAL, NOT_STATIC, NEGATED_FLUENT};
    Type type;
    vector<RuleArg> args;
    int relation = -1;
};

// One step of a compiled join plan.
struct Step {
    int atom;
    bool check_only;
    int index = -1;
    // For index lookups: the key (bound arguments, in position order).
    vector<RuleArg> key;
    // Positions binding new variables (first occurrence) and positions that
    // must equal an already bound argument (bound variables or constants)
    // when iterating a whole relation without index.
    vector<pair<int, int>> new_vars;
    vector<pair<int, RuleArg>> checks;
    vector<int> filters;
};

struct Rule {
    int num_vars;
    vector<RuleAtom> body;
    vector<Filter> filters;
    vector<RuleAtom> heads;
    // plans[i]: join plan when the new atom is at body position i.
    vector<vector<Step>> plans;
    // Filters checkable after matching the trigger atom, per position.
    vector<vector<int>> initial_filters;
};

class Grounder {
    const Task &task;
    vector<Relation> relations;
    vector<Rule> rules;
    // triggers[r]: (rule, body position) pairs whose atom has relation r.
    vector<vector<pair<int, int>>> triggers;
    deque<pair<int, int>> queue;
    vector<pair<int, GroundAtom>> pending_heads;
    size_t total = 0;
    size_t max_total;
    size_t max_operators;
    bool aborted = false;
    //! Count-first mode: the fixpoint derives no instance atoms (rules are
    //! split by variable elimination), the instances are counted and then
    //! enumerated over the final atoms.
    bool split = getenv("PWL_NO_SPLIT_GROUNDING") == nullptr;
    //! If set, fire() collects the first head atom here instead.
    vector<GroundAtom> *collect = nullptr;
    //! If set, fire() collects the values of these variables instead.
    const vector<int> *collect_vars = nullptr;
    //! If set, fire() also collects the time of the binding: the latest
    //! time at which one of its body atoms was added.
    vector<int> *collect_times = nullptr;
    //! time_stack[d]: latest time of the atoms matched before join depth d.
    vector<int> time_stack;
    int clock = 0;

    vector<int> binding;

    bool filter_holds(const Filter &f) const {
        auto value = [&](const RuleArg &a) { return a.is_var ? binding[a.value] : a.value; };
        if (f.type == Filter::EQUAL)
            return value(f.args[0]) == value(f.args[1]);
        if (f.type == Filter::NOT_EQUAL)
            return value(f.args[0]) != value(f.args[1]);
        if (f.type == Filter::NEGATED_FLUENT)
            return true;
        GroundAtom t;
        for (const RuleArg &a : f.args)
            t.push_back(value(a));
        return relations[f.relation].members.count(t) == 0;
    }

    bool filters_hold(const Rule &rule, const vector<int> &filter_ids) const {
        for (int f : filter_ids) {
            if (!filter_holds(rule.filters[f]))
                return false;
        }
        return true;
    }

    void fire(const Rule &rule) {
        if (collect) {
            GroundAtom t;
            if (collect_vars) {
                for (int v : *collect_vars)
                    t.push_back(binding[v]);
            }
            else {
                for (const RuleArg &a : rule.heads[0].args)
                    t.push_back(a.is_var ? binding[a.value] : a.value);
            }
            collect->push_back(std::move(t));
            if (collect_times)
                collect_times->push_back(time_stack.back());
            return;
        }
        if (total + pending_heads.size() > max_total) {
            aborted = true;
            return;
        }
        for (const RuleAtom &h : rule.heads) {
            GroundAtom t;
            for (const RuleArg &a : h.args)
                t.push_back(a.is_var ? binding[a.value] : a.value);
            pending_heads.emplace_back(h.relation, std::move(t));
        }
    }

    void join(const Rule &rule, const vector<Step> &plan, size_t depth) {
        if (aborted)
            return;
        if (depth == plan.size()) {
            if (collect_times)
                time_stack.back() = time_stack[depth];
            fire(rule);
            return;
        }
        const Step &step = plan[depth];
        const RuleAtom &atom = rule.body[step.atom];
        Relation &rel = relations[atom.relation];
        if (step.check_only) {
            GroundAtom t;
            for (const RuleArg &a : atom.args)
                t.push_back(a.is_var ? binding[a.value] : a.value);
            auto it = rel.members.find(t);
            if (it != rel.members.end() && filters_hold(rule, step.filters)) {
                if (collect_times)
                    time_stack[depth + 1] = max(time_stack[depth], it->second);
                join(rule, plan, depth + 1);
            }
            return;
        }
        auto process = [&](const GroundAtom &t, int time) {
            for (const auto &c : step.checks) {
                int v = c.second.is_var ? binding[c.second.value] : c.second.value;
                if (t[c.first] != v)
                    return;
            }
            for (const auto &nv : step.new_vars)
                binding[nv.second] = t[nv.first];
            // Repeated new variables within the atom.
            for (const auto &nv : step.new_vars) {
                if (binding[nv.second] != t[nv.first])
                    return;
            }
            if (filters_hold(rule, step.filters)) {
                if (collect_times)
                    time_stack[depth + 1] = max(time_stack[depth], time);
                join(rule, plan, depth + 1);
            }
        };
        if (step.index >= 0) {
            GroundAtom key;
            for (const RuleArg &a : step.key)
                key.push_back(a.is_var ? binding[a.value] : a.value);
            auto it = rel.indices[step.index].map.find(key);
            if (it == rel.indices[step.index].map.end())
                return;
            // Copy the ids: the list cannot grow during the join (new atoms
            // are buffered), but keep it robust.
            const vector<int> &ids = it->second;
            for (size_t k = 0; k < ids.size(); ++k)
                process(rel.tuples[ids[k]], rel.times[ids[k]]);
        }
        else {
            for (size_t k = 0; k < rel.tuples.size(); ++k)
                process(rel.tuples[k], rel.times[k]);
        }
    }

    bool match_trigger(const Rule &rule, int position, const GroundAtom &t) {
        const RuleAtom &atom = rule.body[position];
        vector<char> bound(rule.num_vars, 0);
        for (size_t i = 0; i < atom.args.size(); ++i) {
            const RuleArg &a = atom.args[i];
            if (!a.is_var) {
                if (t[i] != a.value)
                    return false;
            }
            else if (bound[a.value]) {
                if (binding[a.value] != t[i])
                    return false;
            }
            else {
                bound[a.value] = 1;
                binding[a.value] = t[i];
            }
        }
        return true;
    }

    void compile(Rule &rule);
    int add_relation(int arity) {
        relations.emplace_back();
        relations.back().arity = arity;
        triggers.emplace_back();
        return relations.size() - 1;
    }
    bool normalize(Rule &rule);
    bool add_rule(Rule rule);
    void split_rule(const Rule &rule, vector<Rule> &out);
    //! Enumerate the instances of the first head (stops after `cap`
    //! instances); with by_time, in the order of a semi-naive fixpoint.
    void enumerate(Rule &rule, vector<GroundAtom> &out, bool by_time,
                   size_t cap);
    double count_bindings(const Rule &rule);
    bool build_relaxed_program(const vector<Rule> &action_rules,
                               const vector<pair<int, int>> &rule_schema, RelaxedProgram &program);

public:
    Grounder(const Task &task, size_t max_total, size_t max_operators)
        : task(task), max_total(max_total), max_operators(max_operators) {}

    bool run(RelaxedReachabilityResult &result);
};

void Grounder::compile(Rule &rule) {
    int n = rule.body.size();
    rule.plans.resize(n);
    rule.initial_filters.resize(n);
    auto filter_vars = [&](const Filter &f) {
        vector<int> vars;
        for (const RuleArg &a : f.args) {
            if (a.is_var)
                vars.push_back(a.value);
        }
        return vars;
    };
    for (int i = 0; i < n; ++i) {
        vector<char> bound(rule.num_vars, 0);
        for (const RuleArg &a : rule.body[i].args) {
            if (a.is_var)
                bound[a.value] = 1;
        }
        vector<char> filter_done(rule.filters.size(), 0);
        auto ready_filters = [&]() {
            vector<int> ready;
            for (size_t f = 0; f < rule.filters.size(); ++f) {
                if (filter_done[f])
                    continue;
                bool all = true;
                for (int v : filter_vars(rule.filters[f]))
                    all = all && bound[v];
                if (all) {
                    filter_done[f] = 1;
                    ready.push_back(f);
                }
            }
            return ready;
        };
        rule.initial_filters[i] = ready_filters();
        vector<char> used(n, 0);
        used[i] = 1;
        for (int k = 1; k < n; ++k) {
            // Greedy choice: fully bound atoms first, then the atom with the
            // most bound arguments, then the smaller (static) relation.
            int best = -1;
            tuple<int, int, long long> best_score;
            for (int j = 0; j < n; ++j) {
                if (used[j])
                    continue;
                int num_bound = 0;
                bool all = true;
                for (const RuleArg &a : rule.body[j].args) {
                    if (!a.is_var || bound[a.value])
                        ++num_bound;
                    else
                        all = false;
                }
                long long size = relations[rule.body[j].relation].tuples.size();
                tuple<int, int, long long> score(all, num_bound, -size);
                if (best < 0 || score > best_score) {
                    best = j;
                    best_score = score;
                }
            }
            used[best] = 1;
            Step step;
            step.atom = best;
            const RuleAtom &atom = rule.body[best];
            vector<int> key_positions;
            bool all = true;
            for (size_t p = 0; p < atom.args.size(); ++p) {
                const RuleArg &a = atom.args[p];
                if (!a.is_var || bound[a.value]) {
                    key_positions.push_back(p);
                    step.key.push_back(a);
                }
                else {
                    all = false;
                }
            }
            step.check_only = all;
            if (!all) {
                if (!key_positions.empty())
                    step.index = relations[atom.relation].get_index(key_positions);
                else
                    step.key.clear();
                for (size_t p = 0; p < atom.args.size(); ++p) {
                    const RuleArg &a = atom.args[p];
                    if (a.is_var && !bound[a.value])
                        step.new_vars.emplace_back(p, a.value);
                }
                for (const auto &nv : step.new_vars)
                    bound[nv.second] = 1;
            }
            step.filters = ready_filters();
            rule.plans[i].push_back(std::move(step));
        }
    }
}

bool Grounder::normalize(Rule &rule) {
    // Unify variables of positive equalities and substitute constants.
    vector<int> parent(rule.num_vars);
    iota(parent.begin(), parent.end(), 0);
    vector<int> constant(rule.num_vars, -1);
    function<int(int)> find = [&](int v) { return parent[v] == v ? v : parent[v] = find(parent[v]); };
    vector<Filter> remaining;
    for (const Filter &f : rule.filters) {
        if (f.type != Filter::EQUAL) {
            remaining.push_back(f);
            continue;
        }
        const RuleArg &a = f.args[0], &b = f.args[1];
        if (a.is_var && b.is_var) {
            int x = find(a.value), y = find(b.value);
            if (x != y) {
                if (constant[x] >= 0 && constant[y] >= 0 && constant[x] != constant[y])
                    return false;
                if (constant[x] < 0)
                    constant[x] = constant[y];
                parent[y] = x;
            }
        }
        else if (a.is_var || b.is_var) {
            int v = find(a.is_var ? a.value : b.value);
            int c = a.is_var ? b.value : a.value;
            if (constant[v] >= 0 && constant[v] != c)
                return false;
            constant[v] = c;
        }
        else if (a.value != b.value) {
            return false;
        }
    }
    auto subst = [&](RuleArg &a) {
        if (!a.is_var)
            return;
        int r = find(a.value);
        if (constant[r] >= 0) {
            a.is_var = false;
            a.value = constant[r];
        }
        else {
            a.value = r;
        }
    };
    for (RuleAtom &atom : rule.body)
        for (RuleArg &a : atom.args)
            subst(a);
    for (RuleAtom &atom : rule.heads)
        for (RuleArg &a : atom.args)
            subst(a);
    rule.filters.clear();
    for (Filter f : remaining) {
        for (RuleArg &a : f.args)
            subst(a);
        if (f.type == Filter::NOT_EQUAL && !f.args[0].is_var && !f.args[1].is_var) {
            if (f.args[0].value == f.args[1].value)
                return false;
            continue;
        }
        rule.filters.push_back(f);
    }
    return true;
}

bool Grounder::add_rule(Rule rule) {
    if (!normalize(rule))
        return false;
    int id = rules.size();
    rules.push_back(std::move(rule));
    for (size_t i = 0; i < rules[id].body.size(); ++i)
        triggers[rules[id].body[i].relation].emplace_back(id, i);
    return true;
}

namespace {
vector<int> vars_of(const vector<RuleArg> &args) {
    vector<int> vars;
    for (const RuleArg &a : args) {
        if (a.is_var)
            vars.push_back(a.value);
    }
    sort(vars.begin(), vars.end());
    vars.erase(unique(vars.begin(), vars.end()), vars.end());
    return vars;
}

bool contains(const vector<int> &sorted, int v) {
    return binary_search(sorted.begin(), sorted.end(), v);
}

string describe(const RuleAtom &atom) {
    string d = to_string(atom.relation) + "(";
    for (const RuleArg &a : atom.args)
        d += (a.is_var ? "v" : "c") + to_string(a.value) + ",";
    return d + ")";
}
}  // namespace

/*
  Split a (normalized) rule into rules with smaller joins. The heads are
  grouped by their variables; for each group, the variables not in the
  heads are eliminated one at a time: eliminating X replaces the body atoms
  containing X by an auxiliary atom over their other variables, derived by a
  new rule (as in the size-aware decomposition of CounterAction). We choose
  the variable whose auxiliary atom has the smallest bound on its size (the
  product of the variables' domain sizes, bounded by static unary atoms).
  Filters are checked in the first rule binding all their variables.
*/
void Grounder::split_rule(const Rule &rule, vector<Rule> &out) {
    const double num_objects = max<size_t>(task.objects.size(), 1);
    vector<double> domain(rule.num_vars, num_objects);
    for (const RuleAtom &atom : rule.body) {
        if (atom.args.size() == 1 && atom.args[0].is_var &&
            atom.relation < int(task.predicates.size()) &&
            task.predicates[atom.relation].isStaticPredicate()) {
            // Rules are split before the atoms are loaded.
            double size = task.get_static_info().get_tuples_of_relation(atom.relation).size();
            domain[atom.args[0].value] = min(domain[atom.args[0].value], size);
        }
    }
    map<vector<int>, vector<RuleAtom>> groups;
    for (const RuleAtom &h : rule.heads)
        groups[vars_of(h.args)].push_back(h);
    map<string, int> memo;
    for (const auto &group : groups) {
        const vector<int> &head_vars = group.first;
        vector<RuleAtom> atoms = rule.body;
        vector<Filter> filters = rule.filters;
        while (true) {
            int best = -1;
            tuple<double, size_t, int> best_score;
            vector<int> best_scope;
            for (int v = 0; v < rule.num_vars; ++v) {
                if (contains(head_vars, v))
                    continue;
                vector<int> bucket_vars;
                size_t bucket_size = 0;
                for (const RuleAtom &atom : atoms) {
                    vector<int> av = vars_of(atom.args);
                    if (contains(av, v)) {
                        ++bucket_size;
                        bucket_vars.insert(bucket_vars.end(), av.begin(), av.end());
                    }
                }
                if (bucket_size == 0 || bucket_size == atoms.size())
                    continue;
                sort(bucket_vars.begin(), bucket_vars.end());
                bucket_vars.erase(unique(bucket_vars.begin(), bucket_vars.end()),
                                  bucket_vars.end());
                bool ok = true;
                for (const Filter &f : filters) {
                    vector<int> fv = vars_of(f.args);
                    if (!contains(fv, v))
                        continue;
                    for (int u : fv)
                        ok = ok && contains(bucket_vars, u);
                }
                if (!ok)
                    continue;
                vector<int> scope;
                for (int u : bucket_vars) {
                    if (u != v)
                        scope.push_back(u);
                }
                bool covers_head = true;
                for (int h : head_vars)
                    covers_head = covers_head && contains(scope, h);
                if (covers_head)
                    continue;
                double size = 1;
                for (int u : scope)
                    size *= domain[u];
                auto score = make_tuple(size, scope.size(), v);
                if (best < 0 || score < best_score) {
                    best = v;
                    best_score = score;
                    best_scope = scope;
                }
            }
            if (best < 0)
                break;
            Rule aux;
            aux.num_vars = rule.num_vars;
            vector<RuleAtom> rest;
            vector<int> bucket_vars;
            for (RuleAtom &atom : atoms) {
                vector<int> av = vars_of(atom.args);
                if (contains(av, best)) {
                    bucket_vars.insert(bucket_vars.end(), av.begin(), av.end());
                    aux.body.push_back(std::move(atom));
                }
                else {
                    rest.push_back(std::move(atom));
                }
            }
            sort(bucket_vars.begin(), bucket_vars.end());
            bucket_vars.erase(unique(bucket_vars.begin(), bucket_vars.end()), bucket_vars.end());
            vector<Filter> remaining;
            for (Filter &f : filters) {
                bool covered = true;
                for (int u : vars_of(f.args))
                    covered = covered && contains(bucket_vars, u);
                if (covered)
                    aux.filters.push_back(std::move(f));
                else
                    remaining.push_back(std::move(f));
            }
            filters.swap(remaining);
            vector<string> parts;
            for (const RuleAtom &atom : aux.body)
                parts.push_back(describe(atom));
            sort(parts.begin(), parts.end());
            string signature;
            for (const string &part : parts)
                signature += part;
            for (const Filter &f : aux.filters) {
                RuleAtom fa{f.relation, f.args};
                signature += "|" + to_string(int(f.type)) + describe(fa);
            }
            signature += "->";
            for (int u : best_scope)
                signature += to_string(u) + ",";
            RuleAtom aux_atom;
            for (int u : best_scope)
                aux_atom.args.push_back({true, u});
            auto it = memo.find(signature);
            if (it != memo.end()) {
                aux_atom.relation = it->second;
            }
            else {
                aux_atom.relation = add_relation(best_scope.size());
                memo.emplace(signature, aux_atom.relation);
                aux.heads.push_back(aux_atom);
                out.push_back(std::move(aux));
            }
            rest.push_back(aux_atom);
            atoms.swap(rest);
        }
        Rule final_rule;
        final_rule.num_vars = rule.num_vars;
        final_rule.body = std::move(atoms);
        final_rule.filters = std::move(filters);
        final_rule.heads = group.second;
        out.push_back(std::move(final_rule));
    }
}

/*
  Enumerate the instances of the first head of a rule over the current
  relations with a single join (no fixpoint).
*/
void Grounder::enumerate(Rule &rule, vector<GroundAtom> &out, bool by_time, size_t cap) {
    collect = &out;
    vector<int> times;
    if (by_time) {
        collect_times = &times;
        time_stack.assign(rule.body.size() + 2, 0);
    }
    if (rule.body.empty()) {
        bool ok = true;
        for (const Filter &f : rule.filters) {
            for (const RuleArg &a : f.args)
                ok = ok && !a.is_var;
        }
        vector<int> all(rule.filters.size());
        iota(all.begin(), all.end(), 0);
        if (ok && filters_hold(rule, all))
            fire(rule);
        collect = nullptr;
        collect_times = nullptr;
        return;
    }
    compile(rule);
    int start = 0;
    for (size_t i = 1; i < rule.body.size(); ++i) {
        if (relations[rule.body[i].relation].tuples.size() <
            relations[rule.body[start].relation].tuples.size())
            start = i;
    }
    const Relation &start_rel = relations[rule.body[start].relation];
    for (size_t k = 0; k < start_rel.tuples.size(); ++k) {
        if (!match_trigger(rule, start, start_rel.tuples[k]))
            continue;
        if (!filters_hold(rule, rule.initial_filters[start]))
            continue;
        if (by_time)
            time_stack[0] = start_rel.times[k];
        join(rule, rule.plans[start], 0);
        if (out.size() > cap)
            break;
    }
    collect = nullptr;
    collect_times = nullptr;
    if (by_time) {
        // Order of discovery in a semi-naive fixpoint: by the time at which
        // the last body atom was added.
        vector<int> order(out.size());
        iota(order.begin(), order.end(), 0);
        stable_sort(order.begin(), order.end(),
                    [&](int x, int y) { return times[x] < times[y]; });
        vector<GroundAtom> sorted;
        sorted.reserve(out.size());
        for (int i : order)
            sorted.push_back(std::move(out[i]));
        out.swap(sorted);
    }
}

namespace {
// A function from assignments of its variables to counts.
struct Factor {
    vector<int> vars;
    phmap::flat_hash_map<GroundAtom, double, TupleHash> table;
};

struct TooExpensive {};

Factor join_factors(const Factor &a, const Factor &b, int drop, size_t max_size) {
    const Factor &small = a.table.size() <= b.table.size() ? a : b;
    const Factor &large = a.table.size() <= b.table.size() ? b : a;
    vector<int> shared_small, shared_large, extra;
    for (size_t i = 0; i < small.vars.size(); ++i) {
        auto it = find(large.vars.begin(), large.vars.end(), small.vars[i]);
        if (it != large.vars.end()) {
            shared_small.push_back(i);
            shared_large.push_back(it - large.vars.begin());
        }
    }
    for (size_t j = 0; j < large.vars.size(); ++j) {
        if (find(small.vars.begin(), small.vars.end(), large.vars[j]) == small.vars.end())
            extra.push_back(j);
    }
    vector<int> all_vars = small.vars;
    for (int j : extra)
        all_vars.push_back(large.vars[j]);
    vector<int> keep;
    Factor result;
    for (size_t i = 0; i < all_vars.size(); ++i) {
        if (all_vars[i] != drop) {
            keep.push_back(i);
            result.vars.push_back(all_vars[i]);
        }
    }
    phmap::flat_hash_map<GroundAtom, vector<pair<const GroundAtom *, double>>, TupleHash> index;
    for (const auto &entry : small.table) {
        GroundAtom key;
        for (int i : shared_small)
            key.push_back(entry.first[i]);
        index[key].emplace_back(&entry.first, entry.second);
    }
    GroundAtom key, joined, projected;
    for (const auto &entry : large.table) {
        key.clear();
        for (int j : shared_large)
            key.push_back(entry.first[j]);
        auto it = index.find(key);
        if (it == index.end())
            continue;
        for (const auto &match : it->second) {
            joined = *match.first;
            for (int j : extra)
                joined.push_back(entry.first[j]);
            projected.clear();
            for (int i : keep)
                projected.push_back(joined[i]);
            result.table[projected] += match.second * entry.second;
        }
        if (result.table.size() > max_size)
            throw TooExpensive();
    }
    return result;
}

Factor sum_out(const Factor &f, const vector<char> &drop) {
    Factor result;
    vector<int> keep;
    for (size_t i = 0; i < f.vars.size(); ++i) {
        if (!drop[i]) {
            keep.push_back(i);
            result.vars.push_back(f.vars[i]);
        }
    }
    GroundAtom key;
    for (const auto &entry : f.table) {
        key.clear();
        for (int i : keep)
            key.push_back(entry.first[i]);
        result.table[key] += entry.second;
    }
    return result;
}

double count_by_elimination(vector<Factor> factors, size_t max_size) {
    while (true) {
        for (const Factor &f : factors) {
            if (f.table.empty())
                return 0;
        }
        // Sum out the variables occurring in a single factor.
        map<int, int> occurrences;
        for (const Factor &f : factors)
            for (int v : f.vars)
                ++occurrences[v];
        for (Factor &f : factors) {
            vector<char> drop(f.vars.size(), 0);
            bool any = false;
            for (size_t i = 0; i < f.vars.size(); ++i) {
                if (occurrences[f.vars[i]] == 1) {
                    drop[i] = 1;
                    any = true;
                }
            }
            if (any)
                f = sum_out(f, drop);
        }
        int best = -1;
        tuple<bool, size_t, double> best_score;
        for (const auto &entry : occurrences) {
            if (entry.second < 2)
                continue;
            int v = entry.first;
            vector<int> scope;
            double product = 1, largest = 0;
            for (const Factor &f : factors) {
                if (find(f.vars.begin(), f.vars.end(), v) == f.vars.end())
                    continue;
                scope.insert(scope.end(), f.vars.begin(), f.vars.end());
                product *= max<double>(f.table.size(), 1);
                largest = max<double>(largest, f.table.size());
            }
            sort(scope.begin(), scope.end());
            scope.erase(unique(scope.begin(), scope.end()), scope.end());
            bool fits = false;
            for (const Factor &f : factors) {
                if (find(f.vars.begin(), f.vars.end(), v) == f.vars.end())
                    continue;
                fits = fits || f.vars.size() == scope.size();
            }
            auto score = make_tuple(!fits, scope.size(), fits ? largest : product);
            if (best < 0 || score < best_score) {
                best = v;
                best_score = score;
            }
        }
        if (best < 0)
            break;
        vector<Factor> bucket, rest;
        for (Factor &f : factors) {
            if (find(f.vars.begin(), f.vars.end(), best) != f.vars.end())
                bucket.push_back(std::move(f));
            else
                rest.push_back(std::move(f));
        }
        sort(bucket.begin(), bucket.end(),
             [](const Factor &x, const Factor &y) { return x.table.size() < y.table.size(); });
        Factor product = std::move(bucket[0]);
        for (size_t i = 1; i < bucket.size(); ++i)
            product = join_factors(product, bucket[i], i + 1 == bucket.size() ? best : -1,
                                   max_size);
        rest.push_back(std::move(product));
        factors.swap(rest);
    }
    double result = 1;
    for (const Factor &f : factors) {
        auto it = f.table.find(GroundAtom());
        result *= it == f.table.end() ? 0 : it->second;
    }
    return result;
}
}  // namespace

/*
  Number of bindings of the variables of a (normalized) rule that satisfy
  its body over the current relations, counted by sum-product variable
  elimination without enumerating them. Negative filters are handled by
  inclusion-exclusion (beyond six of them, the remaining ones are ignored,
  which gives an upper bound). Returns -1 if an intermediate factor gets too
  large.
*/
double Grounder::count_bindings(const Rule &rule) {
    vector<const Filter *> negative;
    for (const Filter &f : rule.filters) {
        if (negative.size() < 6)
            negative.push_back(&f);
    }
    const size_t max_size = 4000000;
    double total = 0;
    try {
        for (size_t subset = 0; subset < (size_t(1) << negative.size()); ++subset) {
            // Equalities from NOT_EQUAL filters in the subset: union-find
            // over variables, with constants.
            vector<int> parent(rule.num_vars);
            iota(parent.begin(), parent.end(), 0);
            vector<int> constant(rule.num_vars, -1);
            function<int(int)> find_rep = [&](int v) {
                return parent[v] == v ? v : parent[v] = find_rep(parent[v]);
            };
            bool possible = true;
            vector<RuleAtom> atoms = rule.body;
            int size = 0;
            for (size_t i = 0; i < negative.size(); ++i) {
                if (!(subset >> i & 1))
                    continue;
                ++size;
                const Filter &f = *negative[i];
                if (f.type == Filter::NOT_STATIC) {
                    atoms.push_back({f.relation, f.args});
                    continue;
                }
                const RuleArg &a = f.args[0], &b = f.args[1];
                if (a.is_var && b.is_var) {
                    int x = find_rep(a.value), y = find_rep(b.value);
                    if (x != y) {
                        if (constant[x] >= 0 && constant[y] >= 0 && constant[x] != constant[y])
                            possible = false;
                        if (constant[x] < 0)
                            constant[x] = constant[y];
                        parent[y] = x;
                    }
                }
                else if (a.is_var || b.is_var) {
                    int v = find_rep(a.is_var ? a.value : b.value);
                    int c = a.is_var ? b.value : a.value;
                    if (constant[v] >= 0 && constant[v] != c)
                        possible = false;
                    constant[v] = c;
                }
                else if (a.value != b.value) {
                    possible = false;
                }
            }
            if (!possible)
                continue;
            vector<Factor> factors;
            for (RuleAtom &atom : atoms) {
                for (RuleArg &arg : atom.args) {
                    if (!arg.is_var)
                        continue;
                    int r = find_rep(arg.value);
                    if (constant[r] >= 0)
                        arg = {false, constant[r]};
                    else
                        arg.value = r;
                }
                Factor f;
                vector<int> position_of_var(rule.num_vars, -1);
                for (const RuleArg &arg : atom.args) {
                    if (arg.is_var && position_of_var[arg.value] < 0) {
                        position_of_var[arg.value] = f.vars.size();
                        f.vars.push_back(arg.value);
                    }
                }
                const Relation &rel = relations[atom.relation];
                GroundAtom key;
                auto add_tuple = [&](const GroundAtom &t) {
                    key.resize(f.vars.size());
                    for (int &x : key)
                        x = -1;
                    for (size_t i = 0; i < atom.args.size(); ++i) {
                        const RuleArg &arg = atom.args[i];
                        if (!arg.is_var) {
                            if (t[i] != arg.value)
                                return;
                        }
                        else {
                            int &slot = key[position_of_var[arg.value]];
                            if (slot >= 0 && slot != t[i])
                                return;
                            slot = t[i];
                        }
                    }
                    f.table[key] = 1;
                };
                if (rel.sink) {
                    for (const auto &entry : rel.members)
                        add_tuple(entry.first);
                }
                else {
                    for (const GroundAtom &t : rel.tuples)
                        add_tuple(t);
                }
                factors.push_back(std::move(f));
            }
            double count = count_by_elimination(std::move(factors), max_size);
            total += size % 2 ? -count : count;
        }
    }
    catch (const TooExpensive &) {
        return -1;
    }
    return total;
}

/*
  Relaxed program of the heuristic for tasks with too many ground actions:
  split each rule (action or conditional effect, with all effects as heads
  and negated fluent preconditions as NEGATED_FLUENT filters) and ground the
  parts over the final relations, in order (auxiliary relations are filled
  before the rules using them).
*/
bool Grounder::build_relaxed_program(const vector<Rule> &action_rules,
                                     const vector<pair<int, int>> &rule_schema,
                                     RelaxedProgram &program) {
    const size_t max_instances = 5000000;
    for (size_t k = 0; k < action_rules.size(); ++k) {
        Rule rule = action_rules[k];
        if (!normalize(rule) || rule.heads.empty())
            continue;
        int first_new_relation = relations.size();
        vector<Rule> parts;
        split_rule(rule, parts);
        for (Rule &part : parts) {
            bool is_aux = part.heads[0].relation >= first_new_relation;
            vector<int> vars;
            for (const RuleAtom &atom : part.body)
                for (const RuleArg &a : atom.args)
                    if (a.is_var)
                        vars.push_back(a.value);
            for (const Filter &f : part.filters)
                for (const RuleArg &a : f.args)
                    if (a.is_var)
                        vars.push_back(a.value);
            for (const RuleAtom &atom : part.heads)
                for (const RuleArg &a : atom.args)
                    if (a.is_var)
                        vars.push_back(a.value);
            sort(vars.begin(), vars.end());
            vars.erase(unique(vars.begin(), vars.end()), vars.end());
            vector<GroundAtom> bindings;
            if (part.body.empty() && !vars.empty())
                continue;  // Unbound variables (cannot be grounded).
            collect_vars = &vars;
            enumerate(part, bindings, false, max_instances);
            collect_vars = nullptr;
            if (bindings.size() > max_instances)
                return false;
            int rule_id = program.rules.size();
            program.rules.push_back(
                {rule_schema[k].first, is_aux ? 0 : rule_schema[k].second, !is_aux, vars});
            vector<int> position(rule.num_vars, -1);
            for (size_t i = 0; i < vars.size(); ++i)
                position[vars[i]] = i;
            auto instantiate = [&](const vector<RuleArg> &args, const GroundAtom &values) {
                GroundAtom t;
                for (const RuleArg &a : args)
                    t.push_back(a.is_var ? values[position[a.value]] : a.value);
                return t;
            };
            for (GroundAtom &values : bindings) {
                RelaxedProgram::Instance inst;
                inst.rule = rule_id;
                for (const RuleAtom &atom : part.body)
                    inst.body.emplace_back(atom.relation, instantiate(atom.args, values));
                for (const Filter &f : part.filters) {
                    if (f.type == Filter::NEGATED_FLUENT)
                        inst.body.emplace_back(-1 - f.relation, instantiate(f.args, values));
                }
                for (const RuleAtom &atom : part.heads) {
                    GroundAtom t = instantiate(atom.args, values);
                    if (is_aux)
                        relations[atom.relation].add(t, clock++);
                    inst.heads.emplace_back(atom.relation, std::move(t));
                }
                inst.values = std::move(values);
                program.instances.push_back(std::move(inst));
            }
            if (program.instances.size() > max_instances)
                return false;
        }
    }
    return true;
}

bool Grounder::run(RelaxedReachabilityResult &result) {
    const auto &predicates = task.predicates;
    int num_predicates = predicates.size();
    for (int p = 0; p < num_predicates; ++p)
        add_relation(predicates[p].getArity());

    const auto &schemas = task.get_action_schemas();
    const auto &axioms = task.get_axioms();
    vector<int> action_relation(schemas.size());
    vector<vector<int>> ce_relation(schemas.size());
    vector<int> axiom_relation(axioms.size());
    for (size_t a = 0; a < schemas.size(); ++a) {
        action_relation[a] = add_relation(schemas[a].get_parameters().size());
        for (const ConditionalEffect &ce : schemas[a].get_conditional_effects())
            ce_relation[a].push_back(add_relation(schemas[a].get_parameters().size() +
                                                  ce.parameters.size()));
    }
    for (size_t i = 0; i < axioms.size(); ++i)
        axiom_relation[i] = add_relation(axioms[i].get_parameters().size());

    auto to_rule_atom = [&](const Atom &atom) {
        RuleAtom ra;
        ra.relation = atom.get_predicate_symbol_idx();
        for (const Argument &arg : atom.get_arguments())
            ra.args.push_back({!arg.is_constant(), arg.get_index()});
        return ra;
    };
    auto nullary_atom = [&](int p) {
        RuleAtom ra;
        ra.relation = p;
        return ra;
    };
    auto add_condition = [&](Rule &rule, const Atom &atom) {
        if (atom.get_name() == "=") {
            Filter f;
            f.type = atom.is_negated() ? Filter::NOT_EQUAL : Filter::EQUAL;
            f.args = to_rule_atom(atom).args;
            rule.filters.push_back(f);
        }
        else if (atom.is_negated()) {
            if (predicates[atom.get_predicate_symbol_idx()].isStaticPredicate()) {
                Filter f;
                f.type = Filter::NOT_STATIC;
                f.args = to_rule_atom(atom).args;
                f.relation = atom.get_predicate_symbol_idx();
                rule.filters.push_back(f);
            }
        }
        else {
            rule.body.push_back(to_rule_atom(atom));
        }
    };
    auto add_effects = [&](Rule &rule, const vector<Atom> &effects,
                           const vector<int> &positive_nullary) {
        for (const Atom &eff : effects) {
            if (!eff.is_negated())
                rule.heads.push_back(to_rule_atom(eff));
        }
        for (int p : positive_nullary)
            rule.heads.push_back(nullary_atom(p));
    };

    vector<Rule> empty_body_rules;
    auto register_one = [&](Rule rule) {
        bool empty = rule.body.empty();
        if (empty)
            empty_body_rules.push_back(rule);
        else
            add_rule(std::move(rule));
    };
    // Count-first mode: the rules deriving instance atoms are kept for the
    // enumeration after the fixpoint; the fixpoint uses split rules deriving
    // only the other heads.
    vector<Rule> instance_rules;
    vector<pair<int, int>> instance_rule_owner;  // (kind, index): 0 action, 1 axiom
    vector<vector<Rule>> ce_instance_rules(schemas.size());
    auto register_rule = [&](Rule rule) {
        if (!split) {
            register_one(std::move(rule));
            return;
        }
        Rule instance_rule = rule;
        instance_rule.heads.resize(1);
        rule.heads.erase(rule.heads.begin());
        // num_vars < 0 marks a rule with an unsatisfiable body.
        if (!normalize(instance_rule))
            instance_rule.num_vars = -1;
        instance_rules.push_back(std::move(instance_rule));
        if (rule.heads.empty() || !normalize(rule))
            return;
        vector<Rule> parts;
        split_rule(rule, parts);
        for (Rule &part : parts)
            register_one(std::move(part));
    };

    for (size_t a = 0; a < schemas.size(); ++a) {
        const ActionSchema &schema = schemas[a];
        int num_params = schema.get_parameters().size();
        Rule rule;
        rule.num_vars = num_params + schema.get_fresh_variables().size();
        for (const Atom &pre : schema.get_precondition())
            add_condition(rule, pre);
        for (const Atom &eq : schema.get_static_precondition())
            add_condition(rule, eq);
        const auto &pos_nullary = schema.get_positive_nullary_precond();
        for (size_t p = 0; p < pos_nullary.size(); ++p) {
            if (pos_nullary[p])
                rule.body.push_back(nullary_atom(p));
        }
        RuleAtom instance;
        instance.relation = action_relation[a];
        for (int k = 0; k < num_params; ++k)
            instance.args.push_back({true, k});
        rule.heads.push_back(instance);
        vector<int> nullary_effects;
        const auto &pos_nullary_eff = schema.get_positive_nullary_effects();
        for (size_t p = 0; p < pos_nullary_eff.size(); ++p) {
            if (pos_nullary_eff[p])
                nullary_effects.push_back(p);
        }
        add_effects(rule, schema.get_effects(), nullary_effects);
        Rule schema_rule = rule;
        if (split)
            instance_rule_owner.emplace_back(0, a);
        register_rule(std::move(rule));

        for (size_t c = 0; c < schema.get_conditional_effects().size(); ++c) {
            const ConditionalEffect &ce = schema.get_conditional_effects()[c];
            Rule ce_rule;
            ce_rule.num_vars = num_params + schema.get_fresh_variables().size() +
                ce.parameters.size();
            ce_rule.body.push_back(instance);
            for (const Atom &cond : ce.condition)
                add_condition(ce_rule, cond);
            for (const Atom &eq : ce.equalities)
                add_condition(ce_rule, eq);
            for (int p : ce.positive_nullary_condition)
                ce_rule.body.push_back(nullary_atom(p));
            RuleAtom ce_instance;
            ce_instance.relation = ce_relation[a][c];
            for (int k = 0; k < num_params; ++k)
                ce_instance.args.push_back({true, k});
            for (const Parameter &par : ce.parameters)
                ce_instance.args.push_back({true, par.get_index()});
            ce_rule.heads.push_back(ce_instance);
            add_effects(ce_rule, ce.effects, ce.positive_nullary_effects);
            if (!split) {
                register_rule(std::move(ce_rule));
                continue;
            }
            // The instance rule joins the action instances; the fixpoint
            // rule joins the action's precondition instead.
            Rule ce_instance_rule = ce_rule;
            ce_instance_rule.heads.resize(1);
            if (!normalize(ce_instance_rule))
                ce_instance_rule.num_vars = -1;
            ce_instance_rules[a].push_back(std::move(ce_instance_rule));
            Rule fix_rule;
            fix_rule.num_vars = ce_rule.num_vars;
            fix_rule.body = schema_rule.body;
            fix_rule.filters = schema_rule.filters;
            fix_rule.body.insert(fix_rule.body.end(), ce_rule.body.begin() + 1, ce_rule.body.end());
            fix_rule.filters.insert(fix_rule.filters.end(), ce_rule.filters.begin(),
                                    ce_rule.filters.end());
            fix_rule.heads.assign(ce_rule.heads.begin() + 1, ce_rule.heads.end());
            if (fix_rule.heads.empty() || !normalize(fix_rule))
                continue;
            vector<Rule> parts;
            split_rule(fix_rule, parts);
            for (Rule &part : parts)
                register_one(std::move(part));
        }
    }
    for (size_t i = 0; i < axioms.size(); ++i) {
        const Axiom &axiom = axioms[i];
        Rule rule;
        rule.num_vars = axiom.get_parameters().size();
        for (const Atom &b : axiom.get_body())
            add_condition(rule, b);
        for (const Atom &b : axiom.get_negated_body())
            add_condition(rule, b);
        for (const Atom &eq : axiom.get_equalities())
            add_condition(rule, eq);
        for (int p : axiom.get_positive_nullary_body())
            rule.body.push_back(nullary_atom(p));
        RuleAtom instance;
        instance.relation = axiom_relation[i];
        for (int k = 0; k < rule.num_vars; ++k)
            instance.args.push_back({true, k});
        rule.heads.push_back(instance);
        RuleAtom head;
        head.relation = axiom.get_head_predicate();
        for (int k = 0; k < axiom.get_head_arity(); ++k)
            head.args.push_back({true, k});
        rule.heads.push_back(head);
        if (split)
            instance_rule_owner.emplace_back(1, i);
        register_rule(std::move(rule));
    }

    // Initial atoms: static information, initial state (nullary atoms
    // included, also those of static predicates).
    auto add_atom = [&](int relation, const GroundAtom &t) {
        Relation &rel = relations[relation];
        if (rel.add(t, clock++)) {
            if (!rel.sink)
                queue.emplace_back(relation, rel.tuples.size() - 1);
            ++total;
        }
    };
    const StaticInformation &static_info = task.get_static_info();
    for (int p = 0; p < num_predicates; ++p) {
        for (const GroundAtom &t : static_info.get_tuples_of_relation(p))
            add_atom(p, t);
        for (const GroundAtom &t : task.initial_state.get_tuples_of_relation(p))
            add_atom(p, t);
        if (task.initial_state.get_nullary_atoms()[p])
            add_atom(p, GroundAtom());
    }

    // Compile the join plans (static relation sizes are known now).
    for (Rule &rule : rules)
        compile(rule);
    for (size_t r = 0; r < relations.size(); ++r) {
        if (triggers[r].empty() && relations[r].indices.empty())
            relations[r].sink = true;
    }
    auto export_tuples = [&](int r) {
        Relation &rel = relations[r];
        if (rel.sink || (split && !getenv("PWL_NO_DISCOVERY_ORDER"))) {
            // In the order in which the atoms were added.
            vector<pair<int, const GroundAtom *>> order;
            for (const auto &entry : rel.members)
                order.emplace_back(entry.second, &entry.first);
            sort(order.begin(), order.end());
            vector<GroundAtom> atoms;
            atoms.reserve(order.size());
            for (const auto &entry : order)
                atoms.push_back(*entry.second);
            return atoms;
        }
        return std::move(rel.tuples);
    };

    size_t max_vars = 0;
    for (const Rule &rule : rules)
        max_vars = max(max_vars, size_t(rule.num_vars));
    for (const Rule &rule : empty_body_rules)
        max_vars = max(max_vars, size_t(rule.num_vars));
    binding.assign(max_vars + 1, -1);

    for (const Rule &rule : empty_body_rules) {
        bool ok = true;
        for (const Filter &f : rule.filters) {
            for (const RuleArg &a : f.args)
                ok = ok && !a.is_var;
        }
        if (ok && filters_hold(rule, [&]() {
                vector<int> all(rule.filters.size());
                iota(all.begin(), all.end(), 0);
                return all;
            }()))
            fire(rule);
    }
    for (auto &h : pending_heads)
        add_atom(h.first, h.second);
    pending_heads.clear();

    while (!queue.empty()) {
        auto [relation, tuple_id] = queue.front();
        queue.pop_front();
        // Copy: relations may grow when the pending heads are added below.
        GroundAtom t = relations[relation].tuples[tuple_id];
        for (const auto &trigger : triggers[relation]) {
            const Rule &rule = rules[trigger.first];
            int position = trigger.second;
            if (!match_trigger(rule, position, t))
                continue;
            if (!filters_hold(rule, rule.initial_filters[position]))
                continue;
            join(rule, rule.plans[position], 0);
        }
        if (aborted)
            return false;
        for (auto &h : pending_heads)
            add_atom(h.first, h.second);
        pending_heads.clear();
        if (total > max_total)
            return false;
    }

    if (split) {
        // Joins over the final relations need their tuples.
        for (Relation &rel : relations) {
            if (rel.sink) {
                for (const auto &entry : rel.members) {
                    rel.tuples.push_back(entry.first);
                    rel.times.push_back(entry.second);
                }
                rel.sink = false;
            }
        }
        double counted = 0;
        bool known = true;
        for (size_t k = 0; k < instance_rules.size(); ++k) {
            if (instance_rule_owner[k].first != 0 || instance_rules[k].num_vars < 0)
                continue;
            double c = count_bindings(instance_rules[k]);
            if (c < 0)
                known = false;
            else
                counted += c;
        }
        cout << "Counted operators: " << size_t(counted) << (known ? "" : " (lower bound)")
             << " [" << total << " atoms]" << endl;
        result.counted_operators = counted;
        if (counted > max_operators) {
            result.too_many_operators = true;
            if (getenv("PWL_NO_HYBRID"))
                return false;
            // Relaxed program for the heuristic of the lifted search.
            vector<Rule> program_rules;
            vector<pair<int, int>> rule_schema;
            auto add_negated = [&](Rule &rule, const Atom &atom) {
                if (atom.is_negated() && atom.get_name() != "=" &&
                    !predicates[atom.get_predicate_symbol_idx()].isStaticPredicate()) {
                    Filter f;
                    f.type = Filter::NEGATED_FLUENT;
                    f.args = to_rule_atom(atom).args;
                    f.relation = atom.get_predicate_symbol_idx();
                    rule.filters.push_back(f);
                }
            };
            auto add_negated_nullary = [&](Rule &rule, const vector<bool> &negated) {
                for (size_t p = 0; p < negated.size(); ++p) {
                    if (negated[p] && !predicates[p].isStaticPredicate()) {
                        Filter f;
                        f.type = Filter::NEGATED_FLUENT;
                        f.relation = p;
                        rule.filters.push_back(f);
                    }
                }
            };
            auto add_all_effects = [&](Rule &rule, const vector<Atom> &effects,
                                       const vector<int> &pos_nullary,
                                       const vector<int> &neg_nullary) {
                for (const Atom &eff : effects) {
                    RuleAtom ra = to_rule_atom(eff);
                    if (eff.is_negated())
                        ra.relation = -1 - ra.relation;
                    rule.heads.push_back(ra);
                }
                for (int p : pos_nullary)
                    rule.heads.push_back(nullary_atom(p));
                for (int p : neg_nullary)
                    rule.heads.push_back(nullary_atom(-1 - p));
            };
            for (size_t a = 0; a < schemas.size(); ++a) {
                const ActionSchema &schema = schemas[a];
                Rule rule;
                rule.num_vars = schema.get_parameters().size() + schema.get_fresh_variables().size();
                for (const Atom &pre : schema.get_precondition()) {
                    add_condition(rule, pre);
                    add_negated(rule, pre);
                }
                for (const Atom &eq : schema.get_static_precondition())
                    add_condition(rule, eq);
                const auto &pos_nullary = schema.get_positive_nullary_precond();
                for (size_t p = 0; p < pos_nullary.size(); ++p) {
                    if (pos_nullary[p])
                        rule.body.push_back(nullary_atom(p));
                }
                add_negated_nullary(rule, schema.get_negative_nullary_precond());
                vector<int> pos_eff, neg_eff;
                for (size_t p = 0; p < schema.get_positive_nullary_effects().size(); ++p)
                    if (schema.get_positive_nullary_effects()[p])
                        pos_eff.push_back(p);
                for (size_t p = 0; p < schema.get_negative_nullary_effects().size(); ++p)
                    if (schema.get_negative_nullary_effects()[p])
                        neg_eff.push_back(p);
                Rule action_rule = rule;
                add_all_effects(action_rule, schema.get_effects(), pos_eff, neg_eff);
                program_rules.push_back(action_rule);
                rule_schema.emplace_back(a, schema.get_cost());
                for (const ConditionalEffect &ce : schema.get_conditional_effects()) {
                    Rule ce_rule = rule;
                    ce_rule.num_vars += ce.parameters.size();
                    for (const Atom &cond : ce.condition) {
                        add_condition(ce_rule, cond);
                        add_negated(ce_rule, cond);
                    }
                    for (const Atom &eq : ce.equalities)
                        add_condition(ce_rule, eq);
                    for (int p : ce.positive_nullary_condition)
                        ce_rule.body.push_back(nullary_atom(p));
                    for (int p : ce.negative_nullary_condition) {
                        if (!predicates[p].isStaticPredicate()) {
                            Filter f;
                            f.type = Filter::NEGATED_FLUENT;
                            f.relation = p;
                            ce_rule.filters.push_back(f);
                        }
                    }
                    add_all_effects(ce_rule, ce.effects, ce.positive_nullary_effects,
                                    ce.negative_nullary_effects);
                    program_rules.push_back(ce_rule);
                    rule_schema.emplace_back(a, schema.get_cost());
                }
            }
            auto program = make_shared<RelaxedProgram>();
            if (build_relaxed_program(program_rules, rule_schema, *program)) {
                cout << "Relaxed program: " << program->rules.size() << " rules, "
                     << program->instances.size() << " instances." << endl;
                result.relaxed_program = program;
                result.action_instances.assign(schemas.size(), {});
                result.ce_instances.assign(schemas.size(), {});
                for (size_t a = 0; a < schemas.size(); ++a)
                    result.ce_instances[a].resize(schemas[a].get_conditional_effects().size());
                result.axiom_instances.assign(axioms.size(), {});
                for (size_t k = 0; k < instance_rules.size(); ++k) {
                    if (instance_rules[k].num_vars < 0 || instance_rule_owner[k].first != 1)
                        continue;
                    enumerate(instance_rules[k], result.axiom_instances[instance_rule_owner[k].second],
                              true, max_total);
                }
                result.atoms.resize(num_predicates);
                for (int p = 0; p < num_predicates; ++p)
                    result.atoms[p] = export_tuples(p);
            }
            return false;
        }
        result.action_instances.assign(schemas.size(), {});
        result.ce_instances.assign(schemas.size(), {});
        result.axiom_instances.assign(axioms.size(), {});
        size_t num_instances = 0;
        for (size_t k = 0; k < instance_rules.size(); ++k) {
            if (instance_rules[k].num_vars < 0)
                continue;
            auto [kind, index] = instance_rule_owner[k];
            vector<GroundAtom> &out = kind == 0 ? result.action_instances[index]
                                                : result.axiom_instances[index];
            enumerate(instance_rules[k], out, !getenv("PWL_NO_DISCOVERY_ORDER"), max_operators);
            if (kind == 0)
                num_instances += out.size();
            if (num_instances > max_operators) {
                result.too_many_operators = true;
                return false;
            }
        }
        for (size_t a = 0; a < schemas.size(); ++a) {
            if (ce_instance_rules[a].empty())
                continue;
            Relation &rel = relations[action_relation[a]];
            for (const GroundAtom &t : result.action_instances[a])
                rel.add(t, clock++);
            for (Rule &ce_rule : ce_instance_rules[a]) {
                result.ce_instances[a].emplace_back();
                if (ce_rule.num_vars < 0)
                    continue;
                enumerate(ce_rule, result.ce_instances[a].back(), !getenv("PWL_NO_DISCOVERY_ORDER"),
                          max_operators);
                num_instances += result.ce_instances[a].back().size();
                if (num_instances > max_operators) {
                    result.too_many_operators = true;
                    return false;
                }
            }
        }
        result.atoms.resize(num_predicates);
        for (int p = 0; p < num_predicates; ++p)
            result.atoms[p] = export_tuples(p);
        return true;
    }

    result.atoms.resize(num_predicates);
    for (int p = 0; p < num_predicates; ++p)
        result.atoms[p] = export_tuples(p);
    result.action_instances.resize(schemas.size());
    result.ce_instances.resize(schemas.size());
    for (size_t a = 0; a < schemas.size(); ++a) {
        result.action_instances[a] = export_tuples(action_relation[a]);
        for (int r : ce_relation[a])
            result.ce_instances[a].push_back(export_tuples(r));
    }
    result.axiom_instances.resize(axioms.size());
    for (size_t i = 0; i < axioms.size(); ++i)
        result.axiom_instances[i] = export_tuples(axiom_relation[i]);
    return true;
}
}  // namespace

bool compute_relaxed_reachability(const Task &task, size_t max_instances,
                                  size_t max_operators, RelaxedReachabilityResult &result) {
    Grounder grounder(task, max_instances, max_operators);
    return grounder.run(result);
}
