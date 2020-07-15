#include "lifted_heuristic.h"

#include "arguments.h"
#include "atom.h"
#include "parser.h"
#include "term.h"

#include <iostream>

using namespace std;

LiftedHeuristic::LiftedHeuristic(const Task &task, std::ifstream &in)
    : logic_program(lifted_heuristic::parse_logic_program(in)),
    grounder(logic_program) {
    cout << "Initializing lifted heuristic..." << endl;
}

int LiftedHeuristic::compute_heuristic(const DBState &s, const Task &task) {
    // TODO Compute EDB from state
    // TODO Make new copy of base LP at each iteration (or find another solution)
    // TODO Pass EDB to ground function
    if (task.is_goal(s)) return 0;
    return 1;
}