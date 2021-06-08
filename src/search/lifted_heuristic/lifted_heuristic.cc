#include "lifted_heuristic.h"

#include "arguments.h"
#include "parser.h"
#include "term.h"

#include <iostream>

using namespace std;

LiftedHeuristic::LiftedHeuristic(const Task &task, const std::string &datalog_file, int heuristic_type)
    : logic_program(lifted_heuristic::parse_logic_program(datalog_file)),
    grounder(logic_program, heuristic_type),
    type(heuristic_type)
    {

    useful_nullary_atoms.resize(task.initial_state.get_nullary_atoms().size());
    int pred_idx = 0;
    for (const auto &predicate : task.predicates) {
        useful_atoms[pred_idx] = std::vector<GroundAtom>();
        indices_map.add_predicate_mapping(pred_idx++, logic_program.get_atom_by_name(predicate.getName()));
    }
    for (const auto &object : task.objects) {
        indices_map.add_object_mapping(object.getIndex(), logic_program.get_object_by_name(object.getName()));
    }

    base_fact_index = lifted_heuristic::Fact::get_next_fact_index();

    for (const auto &pp : logic_program.get_map_index_to_atom())
        if (pp.second == "@goal-reachable")
            target_predicate = pp.first;


    if (type == lifted_heuristic::H_ADD)
        cout << "Initializing additive heuristic..." << endl;
    if (type == lifted_heuristic::H_MAX)
        cout << "Initializing h-max heuristic..." << endl;
    if (type == lifted_heuristic::FF)
        cout << "Initializing the FF heuristic..." << endl;
    cout << "Total number of static atoms in the EDB: " << logic_program.get_facts().size() << endl;
    cout << "Total number of rules: " << logic_program.get_rules().size() << endl;
}

int LiftedHeuristic::compute_heuristic(const DBState &s, const Task &task) {
    transform_state_into_edb(s, task.nullary_predicates);

    if (task.is_goal(s)) return 0;
    int h = grounder.ground(logic_program, target_predicate);

    if (type == lifted_heuristic::FF)
        h = grounder.get_ff_value();

    lifted_heuristic::Fact::reset_global_fact_index(base_fact_index);
    logic_program.reset_facts(base_fact_index);
    for (const auto &r : logic_program.get_rules())
        r->clean_up();

    if (h == std::numeric_limits<int>::max())
        return UNSOLVABLE_STATE;
    return h;
}

void LiftedHeuristic::transform_state_into_edb(const DBState &s,
                                               const unordered_set<int> &nullaries) {
    vector<lifted_heuristic::Fact> edb;

    for (const auto &r : s.get_relations()) {
        for (const auto &tuple : r.tuples) {
            vector<pair<int, int>> args;
            args.reserve(tuple.size());
            for (auto obj : tuple) {
                args.emplace_back(indices_map.get_object(obj), lifted_heuristic::OBJECT);
            }
            lifted_heuristic::Arguments arguments(args);
            edb.emplace_back(arguments, indices_map.get_predicate(r.predicate_symbol));
        }
    }

    const vector<bool>& nullary_atoms = s.get_nullary_atoms();
    for (int index : nullaries) {
        if (nullary_atoms[index]) {
            lifted_heuristic::Arguments nullary_arguments;
            edb.emplace_back(nullary_arguments, indices_map.get_predicate(index));
        }
    }

    for (auto &fact : edb) {
        fact.set_fact_index();
        logic_program.insert_fact(fact);
    }
}

void LiftedHeuristic::get_useful_facts(const Task &task, const lifted_heuristic::LogicProgram &lp) {
    // Clean previous useful actions
    for (auto &x : useful_atoms) {
        x.second.clear();
    }

    for (auto && useful_nullary_atom : useful_nullary_atoms)
        useful_nullary_atom = false;

    for (int achiever : grounder.get_best_achievers()) {
        const lifted_heuristic::Fact& fact = lp.get_fact_by_index(achiever);
        if (indices_map.is_auxiliary_predicate(fact.get_predicate_index()))
            continue;
        int task_predicate_index = indices_map.get_inverse_predicate(fact.get_predicate_index());
        GroundAtom ga;
        if (task.nullary_predicates.count(task_predicate_index) > 0) {
            useful_nullary_atoms[task_predicate_index] = true;
        }
        for (const auto  &arg : fact.get_arguments()) {
            ga.push_back(indices_map.get_inverse_object(arg.get_index()));
        }
        useful_atoms[task_predicate_index].push_back(ga);
    }
}
