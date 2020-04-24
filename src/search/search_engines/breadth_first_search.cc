
#include "breadth_first_search.h"

#include "utils.h"
#include "../action.h"
#include "../states/extensional_states.h"
#include "../states/sparse_states.h"
#include "../successor_generators/successor_generator.h"
#include "search_space.h"


#include <iostream>
#include <queue>
#include <vector>

using namespace std;



template <class PackedStateT>
int BreadthFirstSearch<PackedStateT>::search(const Task &task,
                                             SuccessorGenerator *generator,
                                             Heuristic &heuristic)
{
    cout << "Starting breadth first search" << endl;
    clock_t timer_start = clock();
    SparseStatePacker state_packer(task);

    queue<Node> q;  // Queue has Node structures
    segmented_vector::SegmentedVector<pair<int, LiftedOperatorId>> cheapest_parent;
    segmented_vector::SegmentedVector<SparsePackedState> index_to_state;
    unordered_map<SparsePackedState, int, PackedStateHash> visited;

    index_to_state.push_back(state_packer.pack_state(task.initial_state));
    cheapest_parent.push_back(make_pair(-1, LiftedOperatorId(-1, vector<int>())));

    q.emplace(0, 0, this->state_counter);
    visited[state_packer.pack_state(task.initial_state)] = this->state_counter++;

    if (task.is_goal(task.initial_state)) {
        cout << "Initial state is a goal" << endl;
        print_goal_found(*generator, timer_start, this->generations_last_jump);
        extract_plan(
            cheapest_parent, state_packer.pack_state(task.initial_state), visited, index_to_state, state_packer, task);
        return SOLVED;
    }
    while (not q.empty()) {
        Node head = q.front();
        size_t next = head.id;
        int g = head.g;
        q.pop();
        if (this->g_layer < g) {
            this->generations_last_jump = this->generations;
            this->g_layer = g;
        }
        assert(index_to_state.size() >= next);
        State state = state_packer.unpack_state(index_to_state[next]);
        vector<pair<State, LiftedOperatorId>> successors =
            generator->generate_successors(task.actions, state, task.static_info);

        this->generations += successors.size();
        assert(index_to_state.size() == this->state_counter);
        for (const auto &successor : successors) {
            const State &s = successor.first;
            const SparsePackedState &packed = state_packer.pack_state(s);
            const LiftedOperatorId &a = successor.second;
            if (visited.find(packed) == visited.end()) {
                cheapest_parent.push_back(make_pair(next, a));
                q.emplace(g + 1, 0, this->state_counter);
                index_to_state.push_back(packed);
                visited[packed] = this->state_counter;
                if (task.is_goal(s)) {
                    print_goal_found(*generator, timer_start, this->generations_last_jump);
                    extract_plan(
                        cheapest_parent, state_packer.pack_state(s), visited, index_to_state, state_packer, task);
                    return SOLVED;
                }
                this->state_counter++;
            }
        }
    }

    print_no_solution_found(timer_start);

    return NOT_SOLVED;
}

// explicit template instantiations
template class BreadthFirstSearch<SparsePackedState>;
template class BreadthFirstSearch<ExtensionalPackedState>;