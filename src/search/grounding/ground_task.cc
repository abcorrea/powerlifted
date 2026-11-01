#include "ground_task.h"

#include "../action_schema.h"
#include "../task.h"

#include "fast_grounder.h"

#include "../states/state.h"
#include "../utils/timer.h"

#include <algorithm>
#include <malloc.h>
#include <iostream>
#include <new>

using namespace std;

namespace {
void sort_unique(vector<int> &v) {
    sort(v.begin(), v.end());
    v.erase(unique(v.begin(), v.end()), v.end());
}

GroundAtom instantiate_atom(const Atom &atom, const vector<int> &binding) {
    GroundAtom ga;
    ga.reserve(atom.get_arguments().size());
    for (const Argument &arg : atom.get_arguments()) {
        if (arg.is_constant())
            ga.push_back(arg.get_index());
        else
            ga.push_back(binding[arg.get_index()]);
    }
    return ga;
}

bool equalities_hold(const vector<Atom> &equalities, const vector<int> &binding) {
    for (const Atom &eq : equalities) {
        const auto &args = eq.get_arguments();
        int x = args[0].is_constant() ? args[0].get_index() : binding[args[0].get_index()];
        int y = args[1].is_constant() ? args[1].get_index() : binding[args[1].get_index()];
        if ((x == y) == eq.is_negated())
            return false;
    }
    return true;
}
}  // namespace

int GroundTask::add_fact(int predicate, const GroundAtom &args) {
    if (args.empty()) {
        if (nullary_fact[predicate] < 0) {
            nullary_fact[predicate] = facts.size();
            facts.emplace_back(predicate, args);
        }
        return nullary_fact[predicate];
    }
    auto res = atom_to_fact[predicate].try_emplace(args, int(facts.size()));
    if (res.second)
        facts.emplace_back(predicate, args);
    return res.first->second;
}

void GroundTask::get_state_facts(const DBState &state, vector<int> &out) const {
    const auto &relations = state.get_relations();
    for (size_t p = 0; p < relations.size(); ++p) {
        const auto &tuples = relations[p].tuples;
        if (tuples.empty())
            continue;
        const auto &m = atom_to_fact[p];
        for (const GroundAtom &t : tuples) {
            auto it = m.find(t);
            if (it != m.end())
                out.push_back(it->second);
        }
    }
    const auto &nullary = state.get_nullary_atoms();
    for (size_t p = 0; p < nullary.size(); ++p) {
        if (nullary[p] && nullary_fact[p] >= 0)
            out.push_back(nullary_fact[p]);
    }
}

bool GroundTask::build(const Task &task, size_t max_instances, size_t max_operators,
                       double max_seconds) {
    const size_t num_predicates = task.predicates.size();
    atom_to_fact.assign(num_predicates, {});
    nullary_fact.assign(num_predicates, -1);

    const auto &schemas = task.get_action_schemas();
    const auto &task_axioms = task.get_axioms();
    RelaxedReachabilityResult rr;
    utils::Timer exploration_timer;
    bool complete = compute_relaxed_reachability(task, max_instances, max_operators, max_seconds,
                                                 rr);
    cout << "Relaxed reachability analysis: " << exploration_timer() << endl;
    if (rr.timed_out) {
        cout << "Grounding aborted: time limit reached." << endl;
        return false;
    }
    if (!complete && rr.too_many_operators) {
        cout << "Grounding aborted: more than " << max_operators << " operators ("
             << size_t(rr.counted_operators) << " counted)." << endl;
        if (!rr.relaxed_program)
            return false;
        // Keep the facts, axioms, and goal for the relaxed program.
        operators_omitted = true;
        complete = true;
    }
    if (!complete) {
        cout << "Grounding aborted: more than " << max_instances
             << " atoms and instances." << endl;
        return false;
    }

    // Facts: reachable atoms of non-static task predicates.
    for (size_t p = 0; p < num_predicates; ++p) {
        if (task.predicates[p].isStaticPredicate())
            continue;
        for (const GroundAtom &t : rr.atoms[p])
            add_fact(p, t);
    }
    size_t num_action_facts = 0, num_ce_facts = 0;
    for (size_t a = 0; a < schemas.size(); ++a) {
        num_action_facts += rr.action_instances[a].size();
        for (const auto &l : rr.ce_instances[a])
            num_ce_facts += l.size();
    }
    if (!operators_omitted && num_action_facts + num_ce_facts > max_operators) {
        cout << "Grounding aborted: " << num_action_facts << " operators and "
             << num_ce_facts << " conditional effects." << endl;
        return false;
    }

    const StaticInformation &static_info = task.get_static_info();
    auto is_static = [&](int p) { return task.predicates[p].isStaticPredicate(); };
    // Nullary atoms always live in states, also for static predicates; a
    // static nullary atom has the truth value of the initial state.
    const auto &initial_nullary = task.initial_state.get_nullary_atoms();
    auto nullary_status = [&](int p) {
        // 1: true, 0: false (static), 2: fluent (fact id may exist)
        if (is_static(p))
            return initial_nullary[p] ? 1 : 0;
        return 2;
    };
    auto static_holds = [&](int p, const GroundAtom &args) {
        return static_info.get_tuples_of_relation(p).count(args) > 0;
    };

    // Ground operators.
    vector<phmap::flat_hash_map<vector<int>, int, TupleHash>> op_index(schemas.size());
    operators.reserve(num_action_facts);
    vector<int> binding;
    for (size_t a = 0; a < schemas.size() && !operators_omitted; ++a)
    for (const GroundAtom &instance : rr.action_instances[a]) {
        const ActionSchema &schema = schemas[a];
        binding.assign(instance.begin(), instance.end());
        if (!equalities_hold(schema.get_static_precondition(), binding))
            continue;
        GroundOperator op;
        op.schema = a;
        op.cost = schema.get_cost();
        bool applicable = true;
        for (const Atom &pre : schema.get_precondition()) {
            GroundAtom ga = instantiate_atom(pre, binding);
            int p = pre.get_predicate_symbol_idx();
            if (is_static(p)) {
                if (pre.is_negated() && static_holds(p, ga)) {
                    applicable = false;
                    break;
                }
                continue;
            }
            int fact = get_fact(p, ga);
            if (pre.is_negated()) {
                if (fact >= 0)
                    op.negated_pre.push_back(fact);
            }
            else {
                if (fact < 0) {
                    applicable = false;
                    break;
                }
                op.pre.push_back(fact);
            }
        }
        if (!applicable)
            continue;
        const auto &pos_nullary = schema.get_positive_nullary_precond();
        const auto &neg_nullary = schema.get_negative_nullary_precond();
        for (size_t p = 0; p < pos_nullary.size() && applicable; ++p) {
            if (pos_nullary[p]) {
                int status = nullary_status(p);
                if (status == 1)
                    continue;
                if (status == 0 || nullary_fact[p] < 0)
                    applicable = false;
                else
                    op.pre.push_back(nullary_fact[p]);
            }
        }
        for (size_t p = 0; p < neg_nullary.size(); ++p) {
            if (!neg_nullary[p])
                continue;
            int status = nullary_status(p);
            if (status == 1)
                applicable = false;
            else if (status == 2 && nullary_fact[p] >= 0)
                op.negated_pre.push_back(nullary_fact[p]);
        }
        if (!applicable)
            continue;
        for (const Atom &eff : schema.get_effects()) {
            GroundAtom ga = instantiate_atom(eff, binding);
            int p = eff.get_predicate_symbol_idx();
            if (eff.is_negated()) {
                int fact = get_fact(p, ga);
                if (fact >= 0)
                    op.del.push_back(fact);
            }
            else {
                op.add.push_back(add_fact(p, ga));
            }
        }
        const auto &pos_nullary_eff = schema.get_positive_nullary_effects();
        const auto &neg_nullary_eff = schema.get_negative_nullary_effects();
        for (size_t p = 0; p < neg_nullary_eff.size(); ++p) {
            if (neg_nullary_eff[p] && nullary_fact[p] >= 0)
                op.del.push_back(nullary_fact[p]);
        }
        for (size_t p = 0; p < pos_nullary_eff.size(); ++p) {
            if (pos_nullary_eff[p])
                op.add.push_back(add_fact(p, GroundAtom()));
        }
        sort_unique(op.pre);
        sort_unique(op.negated_pre);
        sort_unique(op.add);
        sort_unique(op.del);
        // Add-after-delete: an atom both added and deleted is added.
        vector<int> del;
        set_difference(op.del.begin(), op.del.end(), op.add.begin(), op.add.end(),
                       back_inserter(del));
        op.del.swap(del);
        op.instantiation = binding;
        if (schema.has_conditional_effects())
            op_index[a].emplace(binding, int(operators.size()));
        operators.push_back(std::move(op));
    }

    // Ground conditional effects.
    for (size_t a = 0; a < schemas.size() && !operators_omitted; ++a)
    for (size_t c = 0; c < rr.ce_instances[a].size(); ++c)
    for (const GroundAtom &instance : rr.ce_instances[a][c]) {
        const ActionSchema &schema = schemas[a];
        const ConditionalEffect &ce = schema.get_conditional_effects()[c];
        binding.assign(instance.begin(), instance.end());
        size_t num_params = schema.get_parameters().size();
        vector<int> params(binding.begin(), binding.begin() + num_params);
        auto it = op_index[a].find(params);
        if (it == op_index[a].end())
            continue;
        // Quantified variables are indexed after the parameters and fresh
        // variables; the instance arguments list parameters and then the
        // quantified variables in order.
        vector<int> full_binding(num_params + schema.get_fresh_variables().size()
                                 + ce.parameters.size(), -1);
        for (size_t k = 0; k < num_params; ++k)
            full_binding[k] = binding[k];
        for (size_t k = 0; k < ce.parameters.size(); ++k)
            full_binding[ce.parameters[k].get_index()] = binding[num_params + k];
        if (!equalities_hold(ce.equalities, full_binding))
            continue;
        GroundConditionalEffect gce;
        gce.op = it->second;
        bool possible = true;
        for (const Atom &cond : ce.condition) {
            GroundAtom ga = instantiate_atom(cond, full_binding);
            int p = cond.get_predicate_symbol_idx();
            if (is_static(p)) {
                if (cond.is_negated() && static_holds(p, ga)) {
                    possible = false;
                    break;
                }
                continue;
            }
            int fact = get_fact(p, ga);
            if (cond.is_negated()) {
                if (fact >= 0)
                    gce.negated_condition.push_back(fact);
            }
            else {
                if (fact < 0) {
                    possible = false;
                    break;
                }
                gce.condition.push_back(fact);
            }
        }
        for (int p : ce.positive_nullary_condition) {
            int status = nullary_status(p);
            if (status == 1)
                continue;
            if (status == 0 || nullary_fact[p] < 0)
                possible = false;
            else
                gce.condition.push_back(nullary_fact[p]);
        }
        for (int p : ce.negative_nullary_condition) {
            int status = nullary_status(p);
            if (status == 1)
                possible = false;
            else if (status == 2 && nullary_fact[p] >= 0)
                gce.negated_condition.push_back(nullary_fact[p]);
        }
        if (!possible)
            continue;
        for (const Atom &eff : ce.effects) {
            GroundAtom ga = instantiate_atom(eff, full_binding);
            int p = eff.get_predicate_symbol_idx();
            if (eff.is_negated()) {
                int fact = get_fact(p, ga);
                if (fact >= 0)
                    gce.del.push_back(fact);
            }
            else {
                gce.add.push_back(add_fact(p, ga));
            }
        }
        for (int p : ce.negative_nullary_effects) {
            if (nullary_fact[p] >= 0)
                gce.del.push_back(nullary_fact[p]);
        }
        for (int p : ce.positive_nullary_effects)
            gce.add.push_back(add_fact(p, GroundAtom()));
        sort_unique(gce.condition);
        sort_unique(gce.negated_condition);
        sort_unique(gce.add);
        sort_unique(gce.del);
        operators[gce.op].conditional_effects.push_back(conditional_effects.size());
        conditional_effects.push_back(std::move(gce));
    }

    // Ground axioms.
    for (size_t ax = 0; ax < task_axioms.size(); ++ax)
    for (const GroundAtom &instance : rr.axiom_instances[ax]) {
        const Axiom &axiom = task_axioms[ax];
        binding.assign(instance.begin(), instance.end());
        if (!equalities_hold(axiom.get_equalities(), binding))
            continue;
        GroundAxiom gax;
        gax.stratum = axiom.get_stratum();
        bool possible = true;
        for (const Atom &b : axiom.get_body()) {
            GroundAtom ga = instantiate_atom(b, binding);
            int p = b.get_predicate_symbol_idx();
            if (is_static(p))
                continue;
            int fact = get_fact(p, ga);
            if (fact < 0) {
                possible = false;
                break;
            }
            gax.body.push_back(fact);
        }
        for (const Atom &b : axiom.get_negated_body()) {
            GroundAtom ga = instantiate_atom(b, binding);
            int p = b.get_predicate_symbol_idx();
            if (is_static(p)) {
                if (static_holds(p, ga))
                    possible = false;
                continue;
            }
            int fact = get_fact(p, ga);
            if (fact >= 0)
                gax.negated_body.push_back(fact);
        }
        for (int p : axiom.get_positive_nullary_body()) {
            int status = nullary_status(p);
            if (status == 1)
                continue;
            if (status == 0 || nullary_fact[p] < 0)
                possible = false;
            else
                gax.body.push_back(nullary_fact[p]);
        }
        for (int p : axiom.get_negative_nullary_body()) {
            int status = nullary_status(p);
            if (status == 1)
                possible = false;
            else if (status == 2 && nullary_fact[p] >= 0)
                gax.negated_body.push_back(nullary_fact[p]);
        }
        if (!possible)
            continue;
        GroundAtom head(binding.begin(), binding.begin() + axiom.get_head_arity());
        gax.head = add_fact(axiom.get_head_predicate(), head);
        sort_unique(gax.body);
        sort_unique(gax.negated_body);
        axioms.push_back(std::move(gax));
    }

    // Goal.
    const GoalCondition &goal_condition = task.get_goal();
    for (const AtomicGoal &g : goal_condition.goal) {
        if (is_static(g.get_predicate_index()))
            continue;
        int fact = get_fact(g.get_predicate_index(), g.get_arguments());
        if (g.is_negated()) {
            if (fact >= 0)
                negated_goal.push_back(fact);
        }
        else if (fact < 0) {
            goal_unreachable = true;
        }
        else {
            goal.push_back(fact);
        }
    }
    for (int p : goal_condition.negative_nullary_goals) {
        int status = nullary_status(p);
        if (status == 1)
            goal_unreachable = true;
        else if (status == 2 && nullary_fact[p] >= 0)
            negated_goal.push_back(nullary_fact[p]);
    }
    sort_unique(negated_goal);
    for (int p : goal_condition.positive_nullary_goals) {
        int status = nullary_status(p);
        if (status == 1)
            continue;
        if (status == 0 || nullary_fact[p] < 0)
            goal_unreachable = true;
        else
            goal.push_back(nullary_fact[p]);
    }
    sort_unique(goal);

    get_state_facts(task.initial_state, initial_facts);
    if (operators_omitted) {
        // Relaxed program: encode its atoms as facts.
        const int num_preds = num_predicates;
        vector<phmap::flat_hash_map<GroundAtom, int, TupleHash>> aux_ids;
        auto encode = [&](const RelaxedProgram::ProgramAtom &atom, bool &skip) {
            skip = false;
            int r = atom.first;
            if (r >= num_preds) {
                int k = r - num_preds;
                if (k >= int(aux_ids.size()))
                    aux_ids.resize(k + 1);
                auto it = aux_ids[k].emplace(atom.second, num_relaxation_aux);
                if (it.second)
                    ++num_relaxation_aux;
                return int(facts.size()) + it.first->second;
            }
            int p = r >= 0 ? r : -1 - r;
            if (is_static(p)) {
                skip = true;
                return 0;
            }
            int f = get_fact(p, atom.second);
            if (f < 0) {
                skip = true;
                return 0;
            }
            return r >= 0 ? f : -1 - f;
        };
        // Auxiliary facts are numbered after all facts are known.
        const RelaxedProgram &program = *rr.relaxed_program;
        for (const auto &rule : program.rules)
            relaxation_rules.push_back({rule.schema, rule.cost, rule.final, rule.vars});
        for (const auto &inst : program.instances) {
            RelaxationOperator op;
            op.rule = inst.rule;
            bool skip;
            for (const auto &atom : inst.body) {
                int f = encode(atom, skip);
                if (!skip)
                    op.pre.push_back(f);
            }
            for (const auto &atom : inst.heads) {
                int f = encode(atom, skip);
                if (!skip)
                    op.eff.push_back(f);
            }
            if (op.eff.empty())
                continue;
            sort_unique(op.pre);
            sort_unique(op.eff);
            op.values.assign(inst.values.begin(), inst.values.end());
            relaxation_operators.push_back(std::move(op));
        }
        cout << "Relaxation without ground operators: " << facts.size() << " facts, "
             << relaxation_operators.size() << " relaxed operators, " << num_relaxation_aux
             << " auxiliary facts, " << axioms.size() << " axioms." << endl;
        return false;
    }
    cout << "Grounded task: " << facts.size() << " facts, " << operators.size()
         << " operators, " << conditional_effects.size() << " conditional effects, "
         << axioms.size() << " axioms." << endl;
    cout << "Initial state: " << initial_facts.size() << " facts." << endl;
    return true;
}

static bool grounding_disabled = false;
static size_t max_ground_operators = 3000000;
static double grounding_time_limit = 12;

void GroundTask::disable_grounding() {
    grounding_disabled = true;
}

void GroundTask::set_limits(size_t max_operators, double max_seconds) {
    max_ground_operators = max_operators;
    grounding_time_limit = max_seconds;
}

static shared_ptr<GroundTask> relaxation_cached;

shared_ptr<GroundTask> GroundTask::get_cached_relaxation(const Task &task) {
    get_cached(task);
    return relaxation_cached;
}

shared_ptr<GroundTask> GroundTask::get_cached(const Task &task) {
    static shared_ptr<GroundTask> cached;
    static bool tried = false;
    if (tried)
        return cached;
    tried = true;
    if (grounding_disabled || task.has_object_creation())
        return nullptr;
    utils::Timer timer;
    // Limit on the atoms and instances of the relaxed exploration.
    const size_t max_instances = 4000000;
    auto ground_task = make_shared<GroundTask>();
    bool success = false;
    try {
        success = ground_task->build(task, max_instances, max_ground_operators,
                                     grounding_time_limit);
    }
    catch (const bad_alloc &) {
        cout << "Grounding aborted: out of memory." << endl;
    }
    if (success) {
        cached = ground_task;
    }
    else if (ground_task->operators_omitted) {
        relaxation_cached = ground_task;
    }
    else {
        ground_task.reset();
        // Return the memory of the aborted exploration to the system.
        malloc_trim(0);
    }
    cout << "Grounding time: " << timer() << endl;
    return cached;
}
