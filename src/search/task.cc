#include <utility>

#include <vector>
#include <cassert>
#include <iostream>

#include "task.h"

using namespace std;

const void
Task::addPredicate(const string &name, int index, int arity, bool static_predicate,
                   const vector<int> &types) {
    Task::predicates.emplace_back(name, index, arity, static_predicate, types);
}

const void Task::addObject(const string &name, int index, const vector<int> &types) {
    Task::objects.emplace_back(name, index, types);
}

const void Task::addType(const string &type_name) {
    type_names.push_back(type_name);
}

void Task::initializeEmptyInitialState() {
    /*
     * Creates one empty relation for every predicate of the task in the initial state.
     */
    vector<Relation> vec;
    for (int i = 0; i < predicates.size(); ++i) {
        Relation r;
        r.predicate_symbol = i;
        assert (r.tuples.empty());
        vec.push_back(r);
    }
    initial_state = State(vec);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
void Task::dumpState(State s) {
    /*
     * Output initial state in a human readable way.
     */
    for (int i = 0; i < s.relations.size(); ++i) {
        string relation_name = predicates[i].getName();
        vector<GroundAtom> tuples = s.relations[i].tuples;
        for (auto & tuple : tuples) {
            cout << relation_name << " ";
            for (auto obj : tuple) {
                cout << objects[obj].getName() << " ";
            }
            cout << endl;
        }
    }
}

void Task::dumpGoal() {
    /*
     * Output goal condition in a readable format.
     */
    for (auto g : goal.goal) {
        if (g.negated) {
            cout << "Not ";
        }
        cout << predicates[g.predicate].getName() << " ";
        for (int arg : g.args) {
            cout << objects[arg].getName() << " ";
        }
        cout << endl;
    }
}

#pragma clang diagnostic pop

void Task::initializeGoal(std::vector<AtomicGoal> goals) {
    goal = GoalCondition(std::move(goals));
}

