//
// Created by blaas on 02.07.19.
//

#include <algorithm>
#include <cassert>
#include <queue>
#include <unordered_map>
#include <iostream>

#include "search.h"
#include "successor_generators/successor_generator.h"

using namespace std;

int Search::getNumberExploredStates() const {
    return number_explored_states;
}

int Search::getNumberGeneratedStates() const {
    return number_generated_states;
}

const vector<Action> &Search::search(const Task &task, SuccessorGenerator generator) const {
    /*
     * Simple Breadth first search
     */

    int state_counter = 0;
    queue<int> q;
    unordered_map<int, State> index_to_state;
    unordered_map<int, pair<int, Action>> cheapest_parent;
    index_to_state[0] = task.initial_state;
    cheapest_parent[0] = make_pair(-1, Action());
    q.push(0);


    // TODO add computation of statistics
    while (not q.empty()) {
        int next = q.front();
        q.pop();
        assert (index_to_state.find(next) != index_to_state.end());
        State state = index_to_state[next];
        if (is_goal(state, task.goal)) {
            cout << "Goal state found!" << endl;
            task.dumpState(state);
            // TODO extract plan, somehow.
            return plan;
        }
        // TODO implement successor
        vector<State> successors = generator.generate_successors(task.actions, state);
    }


    return plan;
}

bool Search::is_goal(const State &state, const GoalCondition &goal) const {
    for (const AtomicGoal &atomicGoal : goal.goal) {
        int goal_predicate = atomicGoal.predicate;
        Relation relation_at_goal_predicate = state.relations[goal_predicate];
        assert (goal_predicate == relation_at_goal_predicate.predicate_symbol);
        if (!atomicGoal.negated) {
            // Positive goal
            if (find(relation_at_goal_predicate.tuples.begin(), relation_at_goal_predicate.tuples.end(),
                    atomicGoal.args) == relation_at_goal_predicate.tuples.end()) {
                return false;
            }
        }
        else {
            // Negative goal
            if (find(relation_at_goal_predicate.tuples.begin(), relation_at_goal_predicate.tuples.end(),
                     atomicGoal.args) != relation_at_goal_predicate.tuples.end()) {
                return false;
            }
        }
    }
    return true;
}
