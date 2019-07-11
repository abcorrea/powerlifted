#include <algorithm>
#include <cassert>
#include <ctime>
#include <iostream>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include "search.h"
#include "../successor_generators/successor_generator.h"
#include "../heuristics/goalcount.h"

using namespace std;

int Search::getNumberExploredStates() const {
    return number_explored_states;
}

int Search::getNumberGeneratedStates() const {
    return number_generated_states;
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
        } else {
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
    cout << "Total plan cost:" << states_in_the_plan.size() - 1 << endl;
    while (!states_in_the_plan.empty()) {
        state = states_in_the_plan.top();
        cout << "##################" << endl;
        task.dumpState(state);
        states_in_the_plan.pop();
    }

}

const vector<Action> &Search::search(const Task &task,
                                     SuccessorGenerator generator,
                                     Heuristic &heuristic) const {
    return plan;
}
