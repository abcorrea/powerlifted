
#include "utils.h"
#include "../action.h"
#include "../successor_generators/successor_generator.h"
#include "../states/sparse_states.h"
#include "../task.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

using namespace std;

void print_no_solution_found(const clock_t& timer_start) {
    cerr << "No solution found!" << endl;
    cout << "Total time: " << double(clock() - timer_start) / CLOCKS_PER_SEC << endl;
}


void print_goal_found(
    const SuccessorGenerator &generator,
    const clock_t& timer_start)
{
    cout << "Goal found at: " << double(clock() - timer_start)/CLOCKS_PER_SEC << endl;
    cout << "Total time: " << double(clock() - timer_start)/CLOCKS_PER_SEC << endl;
}

//template<class PackedStateT>
void extract_plan(
    segmented_vector::SegmentedVector<pair<int, LiftedOperatorId>> &cheapest_parent,
    SparsePackedState state,
    const unordered_map<SparsePackedState, int, PackedStateHash> &visited,
    segmented_vector::SegmentedVector<SparsePackedState> &index_to_state,
    const SparseStatePacker &packer,
    const Task &task)
{
    vector<LiftedOperatorId> plan;
    while (cheapest_parent[visited.at(state)].first != -1) {
        plan.push_back(cheapest_parent[visited.at(state)].second);
        state = index_to_state[cheapest_parent[visited.at(state)].first];
    }
    reverse(plan.begin(), plan.end());
    print_plan(plan, task);
}


void print_plan(const std::vector<LiftedOperatorId>& plan, const Task &task) {
    int total_plan_cost = 0;
    int total_plan_length = 0;
    std::ofstream plan_file("sas_plan");
    for (const LiftedOperatorId &a:plan) {
        const ActionSchema &action = task.get_action_schema_by_index(a.get_index());
        total_plan_cost += action.get_cost();
        total_plan_length += 1;
        plan_file << total_plan_length << ": (" << action.get_name() << " ";
        for (const int obj : a.get_instantiation()) {
            plan_file << task.objects[obj].get_name() << " ";
        }
        plan_file << ")\n";
    }
    std::cout << "Total plan cost: " << total_plan_cost << std::endl;
    std::cout << "Plan length: " << total_plan_length << " step(s)." << std::endl;
}
