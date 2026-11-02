#include "ground_task.h"

#include "relaxed_reachability.h"

#include "../action_schema.h"
#include "../states/state.h"
#include "../task.h"
#include "../utils/timer.h"

#include <algorithm>
#include <iostream>
#include <new>
#include <optional>

using namespace std;

namespace {
void sort_unique(vector<int> &v)
{
    sort(v.begin(), v.end());
    v.erase(unique(v.begin(), v.end()), v.end());
}

GroundAtom instantiate_atom(const Atom &atom, const vector<int> &binding)
{
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

bool equalities_hold(const vector<Atom> &equalities, const vector<int> &binding)
{
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

int GroundTask::add_fact(int predicate, const GroundAtom &args)
{
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

void GroundTask::get_state_facts(const DBState &state, vector<int> &out) const
{
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

bool GroundTask::build(const Task &task,
                       size_t max_instances,
                       size_t max_operators,
                       double max_seconds)
{
    const size_t num_predicates = task.predicates.size();
    atom_to_fact.assign(num_predicates, {});
    nullary_fact.assign(num_predicates, -1);

    const auto &schemas = task.get_action_schemas();
    const auto &task_axioms = task.get_axioms();
    RelaxedReachabilityResult reachable;
    utils::Timer exploration_timer;
    ReachabilityOutcome outcome =
        compute_relaxed_reachability(task, max_instances, max_operators, max_seconds, reachable);
    cout << "Relaxed reachability analysis: " << exploration_timer() << endl;
    switch (outcome) {
    case ReachabilityOutcome::TIMED_OUT:
        cout << "Grounding aborted: time limit reached." << endl;
        return false;
    case ReachabilityOutcome::TOO_MANY_ATOMS:
        cout << "Grounding aborted: more than " << max_instances << " atoms and instances." << endl;
        return false;
    case ReachabilityOutcome::TOO_MANY_OPERATORS:
        cout << "Grounding aborted: more than " << max_operators << " operators ("
             << size_t(reachable.counted_operators) << " counted)." << endl;
        if (!reachable.relaxed_program)
            return false;
        // Keep the facts, axioms, and goal for the relaxed program.
        relaxed_program_only = true;
        break;
    case ReachabilityOutcome::COMPLETE:
        break;
    }

    // Facts: reachable atoms of non-static task predicates.
    for (size_t p = 0; p < num_predicates; ++p) {
        if (task.predicates[p].isStaticPredicate())
            continue;
        for (const GroundAtom &t : reachable.atoms[p])
            add_fact(p, t);
    }
    size_t num_action_instances = 0;
    for (const auto &instances : reachable.action_instances)
        num_action_instances += instances.size();

    const StaticInformation &static_info = task.get_static_info();
    auto is_static = [&](int p) { return task.predicates[p].isStaticPredicate(); };
    // Nullary atoms always live in states, also for static predicates; a
    // static nullary atom has the truth value of the initial state.
    const auto &initial_nullary = task.initial_state.get_nullary_atoms();
    enum class NullaryStatus { STATIC_TRUE, STATIC_FALSE, FLUENT };
    auto nullary_status = [&](int p) {
        if (is_static(p))
            return initial_nullary[p] ? NullaryStatus::STATIC_TRUE : NullaryStatus::STATIC_FALSE;
        return NullaryStatus::FLUENT;
    };
    auto static_holds = [&](int p, const GroundAtom &args) {
        return static_info.get_tuples_of_relation(p).count(args) > 0;
    };

    // Ground operators.
    vector<phmap::flat_hash_map<vector<int>, int, TupleHash>> op_index(schemas.size());
    operators.reserve(num_action_instances);
    vector<int> binding;
    for (size_t a = 0; a < schemas.size() && !relaxed_program_only; ++a) {
        for (const GroundAtom &instance : reachable.action_instances[a]) {
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
                    NullaryStatus status = nullary_status(p);
                    if (status == NullaryStatus::STATIC_TRUE)
                        continue;
                    if (status == NullaryStatus::STATIC_FALSE || nullary_fact[p] < 0)
                        applicable = false;
                    else
                        op.pre.push_back(nullary_fact[p]);
                }
            }
            for (size_t p = 0; p < neg_nullary.size(); ++p) {
                if (!neg_nullary[p])
                    continue;
                NullaryStatus status = nullary_status(p);
                if (status == NullaryStatus::STATIC_TRUE)
                    applicable = false;
                else if (status == NullaryStatus::FLUENT && nullary_fact[p] >= 0)
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
            set_difference(
                op.del.begin(), op.del.end(), op.add.begin(), op.add.end(), back_inserter(del));
            op.del.swap(del);
            op.instantiation = binding;
            if (schema.has_conditional_effects())
                op_index[a].emplace(binding, int(operators.size()));
            operators.push_back(std::move(op));
        }
    }

    // Ground conditional effects.
    for (size_t a = 0; a < schemas.size() && !relaxed_program_only; ++a) {
        for (size_t c = 0; c < reachable.ce_instances[a].size(); ++c) {
            for (const GroundAtom &instance : reachable.ce_instances[a][c]) {
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
                vector<int> full_binding(
                    num_params + schema.get_fresh_variables().size() + ce.parameters.size(), -1);
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
                    NullaryStatus status = nullary_status(p);
                    if (status == NullaryStatus::STATIC_TRUE)
                        continue;
                    if (status == NullaryStatus::STATIC_FALSE || nullary_fact[p] < 0)
                        possible = false;
                    else
                        gce.condition.push_back(nullary_fact[p]);
                }
                for (int p : ce.negative_nullary_condition) {
                    NullaryStatus status = nullary_status(p);
                    if (status == NullaryStatus::STATIC_TRUE)
                        possible = false;
                    else if (status == NullaryStatus::FLUENT && nullary_fact[p] >= 0)
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
        }
    }

    // Ground axioms.
    for (size_t ax = 0; ax < task_axioms.size(); ++ax) {
        for (const GroundAtom &instance : reachable.axiom_instances[ax]) {
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
                NullaryStatus status = nullary_status(p);
                if (status == NullaryStatus::STATIC_TRUE)
                    continue;
                if (status == NullaryStatus::STATIC_FALSE || nullary_fact[p] < 0)
                    possible = false;
                else
                    gax.body.push_back(nullary_fact[p]);
            }
            for (int p : axiom.get_negative_nullary_body()) {
                NullaryStatus status = nullary_status(p);
                if (status == NullaryStatus::STATIC_TRUE)
                    possible = false;
                else if (status == NullaryStatus::FLUENT && nullary_fact[p] >= 0)
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
        NullaryStatus status = nullary_status(p);
        if (status == NullaryStatus::STATIC_TRUE)
            goal_unreachable = true;
        else if (status == NullaryStatus::FLUENT && nullary_fact[p] >= 0)
            negated_goal.push_back(nullary_fact[p]);
    }
    sort_unique(negated_goal);
    for (int p : goal_condition.positive_nullary_goals) {
        NullaryStatus status = nullary_status(p);
        if (status == NullaryStatus::STATIC_TRUE)
            continue;
        if (status == NullaryStatus::STATIC_FALSE || nullary_fact[p] < 0)
            goal_unreachable = true;
        else
            goal.push_back(nullary_fact[p]);
    }
    sort_unique(goal);

    get_state_facts(task.initial_state, initial_facts);
    if (relaxed_program_only) {
        /*
          Encode the atoms of the relaxed program as facts (see
          relaxed_program_only); auxiliary facts are numbered after all
          facts. Static atoms and atoms that are not facts are dropped.
        */
        vector<phmap::flat_hash_map<GroundAtom, int, TupleHash>> aux_fact_ids;
        auto encode = [&](const RelaxedProgram::ProgramAtom &atom) -> optional<int> {
            int relation = atom.first;
            if (relation >= int(num_predicates)) {
                size_t aux_relation = relation - num_predicates;
                if (aux_relation >= aux_fact_ids.size())
                    aux_fact_ids.resize(aux_relation + 1);
                auto it =
                    aux_fact_ids[aux_relation].emplace(atom.second, num_relaxed_program_aux_facts);
                if (it.second)
                    ++num_relaxed_program_aux_facts;
                return int(facts.size()) + it.first->second;
            }
            int p = relation >= 0 ? relation : -1 - relation;
            if (is_static(p))
                return nullopt;
            int f = get_fact(p, atom.second);
            if (f < 0)
                return nullopt;
            return relation >= 0 ? f : -1 - f;
        };
        const RelaxedProgram &program = *reachable.relaxed_program;
        relaxed_program_rules = program.rules;
        for (const RelaxedProgram::Instance &instance : program.instances) {
            RelaxedProgramOperator op;
            op.rule = instance.rule;
            for (const auto &atom : instance.body) {
                if (optional<int> f = encode(atom))
                    op.pre.push_back(*f);
            }
            for (const auto &atom : instance.heads) {
                if (optional<int> f = encode(atom))
                    op.eff.push_back(*f);
            }
            if (op.eff.empty())
                continue;
            sort_unique(op.pre);
            sort_unique(op.eff);
            op.values.assign(instance.values.begin(), instance.values.end());
            relaxed_program_operators.push_back(std::move(op));
        }
        cout << "Relaxed program without ground operators: " << facts.size() << " facts, "
             << relaxed_program_operators.size() << " relaxed operators, "
             << num_relaxed_program_aux_facts << " auxiliary facts, " << axioms.size() << " axioms."
             << endl;
        return true;
    }
    cout << "Grounded task: " << facts.size() << " facts, " << operators.size() << " operators, "
         << conditional_effects.size() << " conditional effects, " << axioms.size() << " axioms."
         << endl;
    cout << "Initial state: " << initial_facts.size() << " facts." << endl;
    return true;
}

shared_ptr<GroundTask>
GroundTask::create(const Task &task, size_t max_operators, double max_seconds)
{
    if (task.has_object_creation())
        return nullptr;
    utils::Timer timer;
    // Limit on the atoms and instances of the relaxed exploration.
    const size_t max_instances = 4000000;
    auto ground_task = make_shared<GroundTask>();
    bool success = false;
    try {
        success = ground_task->build(task, max_instances, max_operators, max_seconds);
    }
    catch (const bad_alloc &) {
        cout << "Grounding aborted: out of memory." << endl;
    }
    cout << "Grounding time: " << timer() << endl;
    if (!success)
        return nullptr;
    return ground_task;
}
