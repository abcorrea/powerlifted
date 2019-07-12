#include <iostream>
#include <queue>
#include <vector>

#include "breadth_first_search.h"

using namespace std;

const vector<Action> &BreadthFirstSearch::search(const Task &task,
                                                 SuccessorGenerator generator,
                                                 Heuristic &heuristic) const {
    /*
     * Simple Breadth first search
     */

    cout << "Starting breadth first search" << endl;
    clock_t timer_start = clock();

    int state_counter = 0;
    int generations = 1;
    queue<Node> q; // Queue has Node structures
    unordered_map<int, pair<int, Action>> cheapest_parent;
    unordered_map<int, State> index_to_state;
    unordered_map<State, int, boost::hash<State>> visited;

    unordered_map<int, int> real_dist;

    index_to_state[state_counter] = task.initial_state;
    cheapest_parent[state_counter] = make_pair(-1, Action(-1, vector<int>()));

    int statistics_counter = 0;

    q.emplace(0, 0, state_counter);
    visited[task.initial_state] = state_counter++;

    if (is_goal(task.initial_state, task.goal)) {
        cout << "Initial state is a goal" << endl;
        extract_goal(state_counter, generations, task.initial_state, cheapest_parent, visited, index_to_state, task);
        return plan;
    }

    while (not q.empty()) {
        Node head = q.front();
        int next = head.id;
        int h = head.h;
        q.pop();
        if ((statistics_counter - generations) <= 0) {
            cout << "Expansions " << state_counter << ", generations " << generations << " states at layer " << h
                 << " [" << double(clock() - timer_start) / CLOCKS_PER_SEC << "]" << endl;
            statistics_counter += 50000;
        }
        assert (index_to_state.find(next) != index_to_state.end());
        State state = index_to_state[next];
        vector<pair<State, Action>> successors = generator.generate_successors(task.actions, state, task.static_info);
        //cout << "STATE:" << " ";
        //task.dumpState(state);
        generations += successors.size();
        for (const pair<State, Action> &successor : successors) {
            State s = successor.first;
            Action a = successor.second;
            if (visited.find(s) == visited.end()) {
                cheapest_parent[state_counter] = make_pair(next, a);
                q.emplace(0, 0, state_counter);
                index_to_state[state_counter] = s;
                visited[s] = state_counter;
                state_counter++;
                if (is_goal(s, task.goal)) {
                    extract_goal(state_counter, generations, s, cheapest_parent, visited, index_to_state, task);
                    extract_plan(cheapest_parent, s, visited, index_to_state, task);
                    return plan;
                }
                //cout << "SUCCESSOR:" << " ";
                //task.dumpState(s);
            }
        }
    }


    return plan;
}
