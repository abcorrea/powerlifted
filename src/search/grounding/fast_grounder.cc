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
#include <numeric>

using namespace std;

namespace {
struct Relation {
    int arity = 0;
    vector<GroundAtom> tuples;
    phmap::flat_hash_set<GroundAtom, TupleHash> members;

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

    bool add(const GroundAtom &t) {
        if (!members.insert(t).second)
            return false;
        if (sink)
            return true;
        int id = tuples.size();
        tuples.push_back(t);
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

// Filter: (in)equality between two arguments, or a negated static atom.
struct Filter {
    enum Type {EQUAL, NOT_EQUAL, NOT_STATIC};
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
    bool aborted = false;

    vector<int> binding;

    bool filter_holds(const Filter &f) const {
        auto value = [&](const RuleArg &a) { return a.is_var ? binding[a.value] : a.value; };
        if (f.type == Filter::EQUAL)
            return value(f.args[0]) == value(f.args[1]);
        if (f.type == Filter::NOT_EQUAL)
            return value(f.args[0]) != value(f.args[1]);
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
            if (rel.members.count(t) && filters_hold(rule, step.filters))
                join(rule, plan, depth + 1);
            return;
        }
        auto process = [&](const GroundAtom &t) {
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
            if (filters_hold(rule, step.filters))
                join(rule, plan, depth + 1);
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
                process(rel.tuples[ids[k]]);
        }
        else {
            for (size_t k = 0; k < rel.tuples.size(); ++k)
                process(rel.tuples[k]);
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
    bool add_rule(Rule rule);

public:
    Grounder(const Task &task, size_t max_total) : task(task), max_total(max_total) {}

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

bool Grounder::add_rule(Rule rule) {
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
    int id = rules.size();
    rules.push_back(std::move(rule));
    for (size_t i = 0; i < rules[id].body.size(); ++i)
        triggers[rules[id].body[i].relation].emplace_back(id, i);
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
    auto register_rule = [&](Rule rule) {
        bool empty = rule.body.empty();
        if (empty)
            empty_body_rules.push_back(rule);
        else
            add_rule(std::move(rule));
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
            register_rule(std::move(ce_rule));
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
        register_rule(std::move(rule));
    }

    // Initial atoms: static information, initial state (nullary atoms
    // included, also those of static predicates).
    auto add_atom = [&](int relation, const GroundAtom &t) {
        Relation &rel = relations[relation];
        if (rel.add(t)) {
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
        if (rel.sink)
            return vector<GroundAtom>(rel.members.begin(), rel.members.end());
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
                                  RelaxedReachabilityResult &result) {
    Grounder grounder(task, max_instances);
    return grounder.run(result);
}
