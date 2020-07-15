#include "lifted_heuristic.h"

#include "arguments.h"
#include "atom.h"
#include "fact.h"
#include "parser.h"
#include "term.h"

#include <iostream>

using namespace std;

LiftedHeuristic::LiftedHeuristic(const Task &task, std::ifstream &in)
    : logic_program(lifted_heuristic::parse_logic_program(in)),
    grounder(logic_program) {
    int pred_idx = 0;
    for (const auto &predicate : task.predicates) {
        indices_map.add_predicate_mapping(pred_idx++, logic_program.get_atom_by_name(predicate.getName()));
    }
    int obj_idx = 0;
    for (const auto &object : task.objects) {
        indices_map.add_object_mapping(obj_idx++, object.getIndex());
    }

    base_fact_index = lifted_heuristic::Fact::get_next_fact_index();

    cout << "Initializing lifted heuristic..." << endl;
}

int LiftedHeuristic::compute_heuristic(const DBState &s, const Task &task) {
    vector<lifted_heuristic::Fact> edb = transform_state_into_edb(s, task.nullary_predicates);
    
    lifted_heuristic::LogicProgram lp = logic_program;

    // TODO Pass EDB to ground function
    // TODO Use queue based approach in grounder
    if (task.is_goal(s)) return 0;
    return 1;
}

vector<lifted_heuristic::Fact> LiftedHeuristic::transform_state_into_edb(const DBState &s,
                                                                         const unordered_set<int> &nullaries) {
    vector<lifted_heuristic::Fact> edb;
    lifted_heuristic::Fact::reset_global_fact_index(base_fact_index);

    for (const auto &r : s.get_relations()) {
        for (const auto &tuple : r.tuples) {
            vector<pair<int, int>> args;
            for (auto obj : tuple) {
                args.emplace_back(indices_map.get_object(obj), lifted_heuristic::OBJECT);
            }
            lifted_heuristic::Arguments arguments(args);
            edb.emplace_back(arguments, r.predicate_symbol);
        }
    }

    const vector<bool>& nullary_atoms = s.get_nullary_atoms();
    for (int index : nullaries) {
        if (nullary_atoms[index]) {
            lifted_heuristic::Arguments nullary_arguments;
            edb.emplace_back(nullary_arguments, index);
        }
    }

    for (auto &fact : edb)
        fact.set_fact_index();
    return edb;
}
