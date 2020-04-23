#include "search.h"

#include "../action.h"
#include "../states/extensional_states.h"
#include "../states/sparse_states.h"
#include "../successor_generators/successor_generator.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

using namespace std;


template<class PackedStateT>
void Search<PackedStateT>::extract_plan(segmented_vector::SegmentedVector<pair<int, Action>> &cheapest_parent, SparsePackedState state,
                          unordered_map<SparsePackedState, int, PackedStateHash> &visited,
                          segmented_vector::SegmentedVector<SparsePackedState> &index_to_state,
                          const SparseStatePacker &packer, const Task &task) {
    vector<Action> actions_in_the_plan;
    int total_plan_cost = 0;
    while (cheapest_parent[visited[state]].first != -1) {
        actions_in_the_plan.push_back(cheapest_parent[visited[state]].second);
        state = index_to_state[cheapest_parent[visited[state]].first];
    }
    reverse(actions_in_the_plan.begin(), actions_in_the_plan.end());
    ofstream plan_file("sas_plan");
    for (const Action &a : actions_in_the_plan) {
        total_plan_cost += 1;
        plan_file << '(' << task.actions[a.index].get_name() << " ";
        for (const int obj : a.instantiation) {
            plan_file << task.objects[obj].getName() << " ";
        }
        plan_file << ")\n";
    }
    cout << "Total plan cost: " << total_plan_cost << endl;
}

template<class PackedStateT>
void Search<PackedStateT>::print_no_solution_found(clock_t timer_start) const {
  cout << "Total time: " << double(clock() - timer_start) / CLOCKS_PER_SEC << endl;
  cerr << "No solution found!" << endl;
}

template<class PackedStateT>
void Search<PackedStateT>::print_goal_found(
    const Task &task,
    const SuccessorGenerator *generator,
    clock_t timer_start,
    const SparseStatePacker &state_packer,
    int generations_until_last_jump,
    segmented_vector::SegmentedVector<pair<int, Action>> &cheapest_parent,
    segmented_vector::SegmentedVector<SparsePackedState> &index_to_state,
    unordered_map<SparsePackedState, int, PackedStateHash> &visited,
    const State &state) const {
    cout << "Goal found at: " << double(clock() - timer_start)/CLOCKS_PER_SEC
         << endl;
    cout << "Proportion of time processing cyclic precond: "
         << generator->get_cyclic_time()
             /(double(clock() - timer_start)/CLOCKS_PER_SEC) << endl;
    cout << "Total time: " << double(clock() - timer_start)/CLOCKS_PER_SEC
         << endl;
    cout << "Generations before the last jump: " << generations_until_last_jump
         << endl;
    extract_plan(cheapest_parent, state_packer.pack_state(state),
                 visited,index_to_state, state_packer, task);
}

// explicit template instantiations
template class Search<SparsePackedState>;
template class Search<ExtensionalPackedState>;