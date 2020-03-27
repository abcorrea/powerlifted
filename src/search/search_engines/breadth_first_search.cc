#include "breadth_first_search.h"

#include <iostream>
#include <queue>
#include <vector>

using namespace std;

const int BreadthFirstSearch::search(const Task &task,
                                     SuccessorGenerator *generator,
                                     Heuristic &heuristic) const {
    /*
     * Simple Breadth first search
     */

    cout << "Starting breadth first search" << endl;
    clock_t timer_start = clock();
    StatePacker state_packer(task);

    int state_counter = 0;
    int generations = 0;
    int generations_last_jump = 0;
    int expansions = 0;
    queue<Node> q; // Queue has Node structures
    segmented_vector::SegmentedVector<pair<int, Action>> cheapest_parent;
    segmented_vector::SegmentedVector<PackedState> index_to_state;
    unordered_map<PackedState, int, PackedStateHash> visited;


    index_to_state.push_back(state_packer.pack_state(task.initial_state));
    cheapest_parent.push_back(make_pair(-1, Action(-1, vector<int>())));

    int g_layer = 0;

    q.emplace(0, 0, state_counter);
    visited[state_packer.pack_state(task.initial_state)] = state_counter++;

    if (task.is_goal(task.initial_state, task.goal)) {
        cout << "Initial state is a goal" << endl;
        cout << "Goal found at: " << double(clock() - timer_start) / CLOCKS_PER_SEC << endl;
        extract_goal(state_counter, generations, state_packer.pack_state(task.initial_state),
                cheapest_parent, visited, index_to_state, state_packer, task);
        return SOLVED;
    }

    while (not q.empty()) {
        Node head = q.front();
        int next = head.id;
        int g = head.g;
        expansions++;
        q.pop();
        if (g_layer < g) {
            generations_last_jump = generations;
            g_layer = g;
        }
        assert (index_to_state.size() >= next);
        State state = state_packer.unpack_state(index_to_state[next]);
        vector<pair<State, Action>> successors = generator->generate_successors(task.actions, state, task.static_info);

        generations += successors.size();
        int init_state_succ = 0;
        for (const pair<State, Action> &successor : successors) {
            const State &s = successor.first;
            const PackedState &packed = state_packer.pack_state(s);
            const Action &a = successor.second;
            if (visited.find(packed) == visited.end()) {
                init_state_succ++;
                cheapest_parent.push_back(make_pair(next, a));
                q.emplace(g+1, 0, state_counter);
                index_to_state.push_back(packed);
                visited[packed] = state_counter;
                if (task.is_goal(s, task.goal)) {
                    cout << "Goal found at: " << double(clock() - timer_start) / CLOCKS_PER_SEC << endl;
                    cout << "Proportion of time processing cyclic precond: "
                         << generator->get_cyclic_time()/(double(clock() - timer_start) / CLOCKS_PER_SEC) << endl;
                    cout << "Total number of expansions: " << expansions << endl;
                    cout << "Generations before the last jump: " << generations_last_jump << endl;
                    extract_goal(state_counter, generations, packed,
                            cheapest_parent, visited, index_to_state, state_packer, task);
                    extract_plan(cheapest_parent, packed, visited, index_to_state, state_packer, task);
                    return SOLVED;
                }
                state_counter++;
            }
        }
    }

    cout << generations << endl;
    return NOT_SOLVED;
}
