#include "lifted_heuristic.h"

#include "arguments.h"
#include "atom.h"
#include "parser.h"
#include "term.h"

#include <iostream>

using namespace std;

LiftedHeuristic::LiftedHeuristic(const Task &task, std::ifstream &in)
    : logic_program(lifted_heuristic::parse_logic_program(in)) {
    cout << "Initializing lifted heuristic..." << endl;
}
