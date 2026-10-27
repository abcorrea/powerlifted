#include "ground_task.h"

#include "../action_schema.h"
#include "../task.h"

#include "../datalog/datalog.h"
#include "../datalog/grounder/weighted_grounder.h"
#include "../heuristics/utils.h"
#include "../states/state.h"
#include "../utils/timer.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

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

bool GroundTask::build(const Task &task, size_t max_datalog_facts, size_t max_operators) {
    const size_t num_predicates = task.predicates.size();
    atom_to_fact.assign(num_predicates, {});
    nullary_fact.assign(num_predicates, -1);

    datalog::AnnotationGenerator no_annotation =
        [](int, const Task &) -> unique_ptr<datalog::Annotation> { return nullptr; };
    datalog::Datalog dl(task, no_annotation, true);
    dl.convert_rules_to_normal_form(task);
    dl.set_permanent_edb(task.get_static_info());
    dl.update_rule_indices();
    if (getenv("PWL_DEBUG_GROUNDING"))
        dl.output_rules();

    datalog::WeightedGrounder grounder(dl, datalog::H_MAX, false);
    grounder.set_fact_limit(max_datalog_facts);
    vector<datalog::Fact> state_facts = get_datalog_facts_from_state(task.initial_state, task);
    utils::Timer datalog_timer;
    int result = grounder.ground(dl, state_facts, -1);
    cout << "Datalog exploration: " << dl.get_number_of_facts() << " facts in "
         << datalog_timer() << endl;
    if (result == datalog::WeightedGrounder::GROUNDING_ABORTED) {
        cout << "Grounding aborted: more than " << max_datalog_facts
             << " Datalog facts." << endl;
        return false;
    }

    // Classify the auxiliary instance predicates.
    const auto &schemas = task.get_action_schemas();
    const auto &task_axioms = task.get_axioms();
    int num_dl_predicates = dl.get_number_of_predicates();
    // kind: 0 = other, 1 = action, 2 = conditional effect, 3 = axiom
    vector<int> kind(num_dl_predicates, 0), owner(num_dl_predicates, -1), sub(num_dl_predicates, -1);
    for (size_t a = 0; a < schemas.size(); ++a) {
        int p = dl.get_action_instance_predicate(a);
        if (p >= 0) {
            kind[p] = 1;
            owner[p] = a;
        }
        for (size_t c = 0; c < schemas[a].get_conditional_effects().size(); ++c) {
            int q = dl.get_ce_instance_predicate(a, c);
            kind[q] = 2;
            owner[q] = a;
            sub[q] = c;
        }
    }
    for (size_t i = 0; i < task_axioms.size(); ++i) {
        int p = dl.get_axiom_instance_predicate(i);
        kind[p] = 3;
        owner[p] = i;
    }

    // Facts: reachable atoms of non-static task predicates.
    int num_dl_facts = dl.get_number_of_facts();
    vector<int> action_facts, ce_facts, axiom_facts;
    size_t num_action_facts = 0;
    for (int i = 0; i < num_dl_facts; ++i) {
        const datalog::Fact &f = dl.get_fact_by_index(i);
        int p = f.get_predicate_index();
        if (!f.is_pred_symbol_new()) {
            if (p < int(num_predicates) && !task.predicates[p].isStaticPredicate()) {
                GroundAtom args;
                for (const auto &t : f.get_arguments())
                    args.push_back(t.get_index());
                add_fact(p, args);
            }
            continue;
        }
        if (p >= num_dl_predicates)
            continue;
        if (kind[p] == 1) {
            action_facts.push_back(i);
            ++num_action_facts;
        }
        else if (kind[p] == 2) {
            ce_facts.push_back(i);
        }
        else if (kind[p] == 3) {
            axiom_facts.push_back(i);
        }
    }
    if (num_action_facts + ce_facts.size() > max_operators) {
        cout << "Grounding aborted: " << num_action_facts << " operators and "
             << ce_facts.size() << " conditional effects." << endl;
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
    for (int i : action_facts) {
        const datalog::Fact &f = dl.get_fact_by_index(i);
        int a = owner[f.get_predicate_index()];
        const ActionSchema &schema = schemas[a];
        binding.clear();
        for (const auto &t : f.get_arguments())
            binding.push_back(t.get_index());
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
        op_index[a].emplace(binding, int(operators.size()));
        operators.push_back(std::move(op));
    }

    // Ground conditional effects.
    for (int i : ce_facts) {
        const datalog::Fact &f = dl.get_fact_by_index(i);
        int a = owner[f.get_predicate_index()];
        int c = sub[f.get_predicate_index()];
        const ActionSchema &schema = schemas[a];
        const ConditionalEffect &ce = schema.get_conditional_effects()[c];
        binding.clear();
        for (const auto &t : f.get_arguments())
            binding.push_back(t.get_index());
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
    for (int i : axiom_facts) {
        const datalog::Fact &f = dl.get_fact_by_index(i);
        const Axiom &axiom = task_axioms[owner[f.get_predicate_index()]];
        binding.clear();
        for (const auto &t : f.get_arguments())
            binding.push_back(t.get_index());
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

    if (getenv("PWL_DEBUG_GROUNDING")) {
        vector<int> per_schema(schemas.size(), 0), per_schema_kept(schemas.size(), 0);
        for (int i : action_facts)
            ++per_schema[owner[dl.get_fact_by_index(i).get_predicate_index()]];
        for (const auto &op : operators)
            ++per_schema_kept[op.schema];
        for (size_t a = 0; a < schemas.size(); ++a)
            cout << "  schema " << schemas[a].get_name() << ": " << per_schema[a]
                 << " instances, " << per_schema_kept[a] << " operators" << endl;
    }
    cout << "Grounded task: " << facts.size() << " facts, " << operators.size()
         << " operators, " << conditional_effects.size() << " conditional effects, "
         << axioms.size() << " axioms." << endl;
    return true;
}

static bool grounding_disabled = false;

void GroundTask::disable_grounding() {
    grounding_disabled = true;
}

shared_ptr<GroundTask> GroundTask::get_cached(const Task &task) {
    static shared_ptr<GroundTask> cached;
    static bool tried = false;
    if (tried)
        return cached;
    tried = true;
    if (grounding_disabled || getenv("PWL_NO_GROUNDING") || task.has_object_creation())
        return nullptr;
    utils::Timer timer;
    auto ground_task = make_shared<GroundTask>();
    if (ground_task->build(task, 4000000, 2000000)) {
        cached = ground_task;
    }
    cout << "Grounding time: " << timer() << endl;
    return cached;
}
