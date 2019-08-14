#include <algorithm>
#include <cassert>
#include <ctime>
#include <fstream>
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

void Search::extract_goal(int state_counter, int generations, State state,
                          unordered_map<int, pair<int, Action>> &cheapest_parent,
                          unordered_map<State, int, boost::hash<State>> &visited,
                          unordered_map<int, State> &index_to_state, const Task &task) const {
    cout << "Goal state found!" << endl;
    cout << "Total number of states visited:" << visited.size() << endl;
    cout << "Total number of states generated:" << generations << endl;
    stack<State> states_in_the_plan;
    states_in_the_plan.push(state);
    while (cheapest_parent[visited[state]].first != -1) {
        state = index_to_state[cheapest_parent[visited[state]].first];
        states_in_the_plan.push(state);
    }
    cout << "Total plan cost:" << states_in_the_plan.size() - 1 << endl;
    /* The section below prints the states on the plan found.
     while (!states_in_the_plan.empty()) {
        state = states_in_the_plan.top();
        cout << "##################" << endl;
        task.dumpState(state);
        states_in_the_plan.pop();
    }*/

}

const int Search::search(const Task &task,
                         SuccessorGenerator *generator,
                         Heuristic &heuristic) const {
    return NOT_SOLVED;
}

void Search::extract_plan(unordered_map<int, pair<int, Action>> &cheapest_parent, State state,
                                    unordered_map<State, int, boost::hash<State>> &visited,
                                    unordered_map<int, State> &index_to_state,
                                    const Task &task) {
    vector<Action> actions_in_the_plan;
    while (cheapest_parent[visited[state]].first != -1) {
        actions_in_the_plan.push_back(cheapest_parent[visited[state]].second);
        state = index_to_state[cheapest_parent[visited[state]].first];
    }
    reverse(actions_in_the_plan.begin(), actions_in_the_plan.end());
    ofstream plan_file("sas_plan");
    for (const Action &a : actions_in_the_plan) {
        plan_file << '(' << task.actions[a.index].getName() << " ";
        for (const int obj : a.instantiation) {
            plan_file << task.objects[obj].getName() << " ";
        }
        plan_file << ")\n";
    }
}

