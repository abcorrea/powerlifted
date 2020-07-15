#include "lifted_heuristic.h"

#include "arguments.h"
#include "atom.h"
#include "term.h"

#include <iostream>

using namespace std;

LiftedHeuristic::LiftedHeuristic(const Task &task) {
    cout << "Initializing lifted heuristic..." << endl;

    for (const ActionSchema &action_schema : task.actions) {
        for (const auto &eff : action_schema.get_effects()) {
            vector<pair<int, int>> arguments;
            for (const auto &arg : eff.arguments) {
                if (arg.constant) arguments.emplace_back(arg.index, lifted_heuristic::OBJECT);
                else arguments.emplace_back(arg.index, lifted_heuristic::VARIABLE);
            }

            lifted_heuristic::Atom head_atom(lifted_heuristic::Arguments(arguments), eff.predicate_symbol);
        }
    }

    cout << "IDB initialized..." << endl;
}
