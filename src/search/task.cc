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
    vector<Relation> fluents, static_preds;
    for (int i = 0; i < predicates.size(); ++i) {
        Relation r;
        r.predicate_symbol = i;
        assert (r.tuples.empty());
        static_preds.push_back(r);
        fluents.push_back(r);
    }
    initial_state = State(fluents, vector<bool>(predicates.size(), false));
    static_info = StaticInformation(static_preds, vector<bool>(predicates.size(), false));
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
void Task::dumpState(State s) const {
    /*
     * Output initial state in a human readable way.
     */
    for (int i = 0; i < s.relations.size(); ++i) {
        string relation_name = predicates[i].getName();
        unordered_set<GroundAtom, TupleHash> tuples = s.relations[i].tuples;
        for (auto & tuple : tuples) {
            cout << relation_name << " ";
            for (auto obj : tuple) {
                cout << objects[obj].getName() << ", ";
            }
        }
    }
    cout << endl;
}

void Task::dumpGoal() {
    /*
     * Output goal condition in a readable format.
     */
    for (const auto& g : goal.goal) {
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

void Task::initializeActionSchemas(const std::vector<ActionSchema>& action_list) {
    actions = action_list;
}

bool Task::is_goal(const State &state, const GoalCondition &goal_condition) const {
    for (const AtomicGoal &atomicGoal : goal_condition.goal) {
        int goal_predicate = atomicGoal.predicate;
        Relation relation_at_goal_predicate = state.relations[goal_predicate];
        if (predicates[relation_at_goal_predicate.predicate_symbol].isStaticPredicate())
            continue;
        assert (goal_predicate == relation_at_goal_predicate.predicate_symbol);
        if (!atomicGoal.negated) {
            // Positive goal_condition
            if (find(relation_at_goal_predicate.tuples.begin(), relation_at_goal_predicate.tuples.end(),
                     atomicGoal.args) == relation_at_goal_predicate.tuples.end()) {
                return false;
            }
        } else {
            // Negative goal_condition
            if (find(relation_at_goal_predicate.tuples.begin(), relation_at_goal_predicate.tuples.end(),
                     atomicGoal.args) != relation_at_goal_predicate.tuples.end()) {
                return false;
            }
        }
    }
    return true;
}

bool Task::is_trivially_unsolvable() const {
    /*
     * Checks whether the static conditions in the goal condition are not satisfied
     */
    for (const AtomicGoal &atomicGoal : goal.goal) {
        int goal_predicate = atomicGoal.predicate;
        Relation relation_at_goal_predicate = static_info.relations[goal_predicate];
        if (!predicates[relation_at_goal_predicate.predicate_symbol].isStaticPredicate())
            continue;
        assert (goal_predicate == relation_at_goal_predicate.predicate_symbol);
        if (!atomicGoal.negated) {
            // Positive goal_condition
            if (find(relation_at_goal_predicate.tuples.begin(), relation_at_goal_predicate.tuples.end(),
                     atomicGoal.args) == relation_at_goal_predicate.tuples.end()) {
                return true;
            }
        }
        else {
            // Negative goal_condition
            if (find(relation_at_goal_predicate.tuples.begin(), relation_at_goal_predicate.tuples.end(),
                     atomicGoal.args) != relation_at_goal_predicate.tuples.end()) {
                return true;
            }
        }
    }
    return false;
}
