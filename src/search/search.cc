//
// Created by blaas on 02.07.19.
//

#include <algorithm>
#include <cassert>
#include <iostream>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include "search.h"
#include "successor_generators/successor_generator.h"
#include "heuristics/goalcount.h"

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
    int generations = 1;
    queue<Node> q; // Queue has Node structures
    unordered_map<int, pair<int, Action>> cheapest_parent;
    unordered_map<int, State> index_to_state;
    unordered_map<State, int, boost::hash<State>> visited;

    unordered_map<int, int> real_dist;

    index_to_state[state_counter] = task.initial_state;
    cheapest_parent[state_counter] = make_pair(-1, Action());

    int statistics_counter = 0;

    q.emplace(0, 0, state_counter);
    visited[task.initial_state] = state_counter++;

    while (not q.empty()) {
        Node head = q.front();
        int next = head.id;
        int h = head.h;
        q.pop();
        if ((statistics_counter - generations) <=  0) {
            cout << "Expanded " << state_counter << " states at layer " << h << endl;
            cout << "Generate " << generations << " states at layer " << h << endl;
            statistics_counter += 10000;
        }
        assert (index_to_state.find(next) != index_to_state.end());
        State state = index_to_state[next];
        if (is_goal(state, task.goal)) {
            extract_goal(state_counter, generations, state, cheapest_parent, visited, index_to_state, task);
            // TODO extract plan, somehow.
            return plan;
        }
        vector<State> successors = generator.generate_successors(task.actions, state, task.static_info);
        for (const State &s : successors) {
            // TODO implement the heuristic and g-values update correctly
            generations++;
            if (visited.find(s) == visited.end()) {
                cheapest_parent[state_counter] = make_pair(next, Action());
                q.emplace(0, 0, state_counter);
                index_to_state[state_counter] = s;
                visited[s] = state_counter;
                state_counter++;
            }
        }
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

void Search::extract_goal(int state_counter, int generations, State state,
                          unordered_map<int, pair<int, Action>> &cheapest_parent,
                          unordered_map<State, int, boost::hash<State>> &visited,
                          unordered_map<int, State> &index_to_state, const Task &task) const {
    cout << "Goal state found!" << endl;
    cout << "Total number of states visited:" << state_counter << endl;
    cout << "Total number of states generated:" << generations << endl;
    stack<State> states_in_the_plan;
    states_in_the_plan.push(state);
    while (cheapest_parent[visited[state]].first != -1) {
        state = index_to_state[cheapest_parent[visited[state]].first];
        states_in_the_plan.push(state);
    }
    cout << "Total plan cost:" << states_in_the_plan.size() -1 << endl;
    while (!states_in_the_plan.empty()) {
        state = states_in_the_plan.top();
        cout << "##################" << endl;
        task.dumpState(state);
        states_in_the_plan.pop();
    }

}