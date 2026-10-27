#include "conditional_effects.h"

#include "../action_schema.h"
#include "../states/state.h"
#include "../task.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <tuple>

using namespace std;

ConditionalEffectEvaluator::ConditionalEffectEvaluator(const Task &task)
    : static_information(task.get_static_info())
{
    for (const auto &r : static_information.get_relations()) {
        is_static.push_back(!r.tuples.empty());
    }
    for (const ActionSchema &action : task.get_action_schemas()) {
        compiled.emplace_back();
        for (const ConditionalEffect &ce : action.get_conditional_effects()) {
            compiled.back().push_back(compile(action, ce));
        }
    }
}

ConditionalEffectEvaluator::CompiledConditionalEffect
ConditionalEffectEvaluator::compile(const ActionSchema &action,
                                    const ConditionalEffect &ce) const
{
    CompiledConditionalEffect result;
    int base = action.get_parameters().size() + action.get_fresh_variables().size();
    result.num_slots = base + ce.parameters.size();
    for (const Parameter &p : ce.parameters) {
        assert(p.index >= base && p.index < result.num_slots);
        (void)p;
    }
    vector<bool> bound(result.num_slots, false);
    for (int i = 0; i < base; ++i)
        bound[i] = true;

    auto compile_atom = [&](const Atom &atom, bool bind) {
        CompiledAtom c;
        c.predicate = atom.get_predicate_symbol_idx();
        c.negated = atom.is_negated();
        c.use_static = c.predicate < int(is_static.size()) && is_static[c.predicate];
        c.fully_bound = true;
        vector<int> seen;
        for (const Argument &arg : atom.get_arguments()) {
            CompiledArgument a;
            if (arg.is_constant()) {
                a.kind = CompiledArgument::CONSTANT;
                a.value = arg.get_index();
            }
            else {
                a.value = arg.get_index();
                if (bound[a.value]) {
                    a.kind = CompiledArgument::BOUND;
                }
                else if (find(seen.begin(), seen.end(), a.value) != seen.end()) {
                    a.kind = CompiledArgument::REPEATED;
                }
                else {
                    a.kind = CompiledArgument::NEW;
                    seen.push_back(a.value);
                    c.fully_bound = false;
                }
            }
            c.arguments.push_back(a);
        }
        if (bind) {
            for (int v : seen)
                bound[v] = true;
        }
        return c;
    };

    // Order the positive atoms greedily: atoms with all variables bound
    // first (lookups), then those with most bound arguments, then those
    // binding most new variables, preferring fluent relations.
    vector<const Atom *> remaining;
    for (const Atom &atom : ce.condition) {
        if (!atom.is_negated())
            remaining.push_back(&atom);
    }
    while (!remaining.empty()) {
        size_t best = 0;
        tuple<int, int, int, int> best_score(-1, -1, -1, -1);
        for (size_t i = 0; i < remaining.size(); ++i) {
            int num_bound = 0, num_new = 0;
            bool all_bound = true;
            vector<int> seen;
            for (const Argument &arg : remaining[i]->get_arguments()) {
                if (arg.is_constant() || bound[arg.get_index()]) {
                    ++num_bound;
                }
                else {
                    all_bound = false;
                    if (find(seen.begin(), seen.end(), arg.get_index()) == seen.end()) {
                        seen.push_back(arg.get_index());
                        ++num_new;
                    }
                }
            }
            int pred = remaining[i]->get_predicate_symbol_idx();
            int fluent = (pred < int(is_static.size()) && is_static[pred]) ? 0 : 1;
            tuple<int, int, int, int> score(all_bound, num_bound, num_new, fluent);
            if (score > best_score) {
                best_score = score;
                best = i;
            }
        }
        result.positive.push_back(compile_atom(*remaining[best], true));
        remaining.erase(remaining.begin() + best);
    }
    for (int v = 0; v < result.num_slots; ++v) {
        if (!bound[v]) {
            cerr << "Error: quantified variable of a conditional effect of action "
                 << action.get_name() << " does not occur in a positive condition atom."
                 << endl;
            exit(-1);
        }
    }
    for (const Atom &atom : ce.condition) {
        if (atom.is_negated())
            result.negative.push_back(compile_atom(atom, false));
    }
    for (const Atom &atom : ce.equalities) {
        result.equalities.push_back(compile_atom(atom, false));
    }
    for (const Atom &atom : ce.effects) {
        result.effects.push_back(compile_atom(atom, false));
    }
    result.positive_nullary_condition = ce.positive_nullary_condition;
    result.negative_nullary_condition = ce.negative_nullary_condition;
    result.positive_nullary_effects = ce.positive_nullary_effects;
    result.negative_nullary_effects = ce.negative_nullary_effects;
    return result;
}

GroundAtom ConditionalEffectEvaluator::instantiate(const CompiledAtom &atom,
                                                   const vector<int> &binding)
{
    GroundAtom ga;
    ga.reserve(atom.arguments.size());
    for (const CompiledArgument &a : atom.arguments) {
        if (a.kind == CompiledArgument::CONSTANT)
            ga.push_back(a.value);
        else
            ga.push_back(binding[a.value]);
    }
    return ga;
}

bool ConditionalEffectEvaluator::check_remaining(const CompiledConditionalEffect &ce,
                                                 const vector<int> &binding,
                                                 const DBState &state) const
{
    for (const CompiledAtom &eq : ce.equalities) {
        int x = eq.arguments[0].kind == CompiledArgument::CONSTANT
            ? eq.arguments[0].value : binding[eq.arguments[0].value];
        int y = eq.arguments[1].kind == CompiledArgument::CONSTANT
            ? eq.arguments[1].value : binding[eq.arguments[1].value];
        if ((x == y) == eq.negated)
            return false;
    }
    for (const CompiledAtom &atom : ce.negative) {
        const auto &tuples = atom.use_static
            ? static_information.get_tuples_of_relation(atom.predicate)
            : state.get_tuples_of_relation(atom.predicate);
        if (tuples.empty())
            continue;
        if (tuples.count(instantiate(atom, binding)))
            return false;
    }
    return true;
}

void ConditionalEffectEvaluator::match(const CompiledConditionalEffect &ce,
                                       size_t depth,
                                       vector<int> &binding,
                                       const DBState &state,
                                       vector<pair<int, GroundAtom>> &deletes,
                                       vector<pair<int, GroundAtom>> &adds) const
{
    if (depth == ce.positive.size()) {
        if (!check_remaining(ce, binding, state))
            return;
        for (const CompiledAtom &eff : ce.effects) {
            if (eff.negated)
                deletes.emplace_back(eff.predicate, instantiate(eff, binding));
            else
                adds.emplace_back(eff.predicate, instantiate(eff, binding));
        }
        for (int p : ce.negative_nullary_effects)
            deletes.emplace_back(p, GroundAtom());
        for (int p : ce.positive_nullary_effects)
            adds.emplace_back(p, GroundAtom());
        return;
    }
    const CompiledAtom &atom = ce.positive[depth];
    const auto &tuples = atom.use_static
        ? static_information.get_tuples_of_relation(atom.predicate)
        : state.get_tuples_of_relation(atom.predicate);
    if (tuples.empty())
        return;
    if (atom.fully_bound) {
        if (tuples.count(instantiate(atom, binding)))
            match(ce, depth + 1, binding, state, deletes, adds);
        return;
    }
    const size_t arity = atom.arguments.size();
    for (const GroundAtom &t : tuples) {
        bool ok = true;
        for (size_t i = 0; i < arity && ok; ++i) {
            const CompiledArgument &a = atom.arguments[i];
            switch (a.kind) {
            case CompiledArgument::CONSTANT:
                ok = (t[i] == a.value);
                break;
            case CompiledArgument::BOUND:
            case CompiledArgument::REPEATED:
                ok = (t[i] == binding[a.value]);
                break;
            case CompiledArgument::NEW:
                binding[a.value] = t[i];
                break;
            }
        }
        if (ok)
            match(ce, depth + 1, binding, state, deletes, adds);
    }
}

void ConditionalEffectEvaluator::evaluate(const ActionSchema &action,
                                          const vector<int> &instantiation,
                                          const unordered_map<int, int> &fresh_vars,
                                          const DBState &state,
                                          vector<pair<int, GroundAtom>> &deletes,
                                          vector<pair<int, GroundAtom>> &adds) const
{
    const auto &nullary = state.get_nullary_atoms();
    for (const CompiledConditionalEffect &ce : compiled[action.get_index()]) {
        bool ok = true;
        for (int p : ce.positive_nullary_condition)
            ok = ok && nullary[p];
        for (int p : ce.negative_nullary_condition)
            ok = ok && !nullary[p];
        if (!ok)
            continue;
        vector<int> binding(ce.num_slots, -1);
        for (size_t i = 0; i < instantiation.size(); ++i)
            binding[i] = instantiation[i];
        for (const auto &entry : fresh_vars)
            binding[entry.first] = entry.second;
        match(ce, 0, binding, state, deletes, adds);
    }
}
