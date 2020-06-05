#include "greedy_best_first_search.h"

#include "../action.h"
#include "../heuristics/heuristic.h"
#include "../states/extensional_states.h"
#include "../states/sparse_states.h"
#include "../successor_generators/successor_generator.h"
#include "search.h"
#include "../task.h"
#include "utils.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <vector>

using namespace std;

template <class PackedStateT>
utils::ExitCode GreedyBestFirstSearch<PackedStateT>::search(const Task &task,
                                                SuccessorGenerator &generator,
                                                Heuristic &heuristic)
{
    cout << "Starting greedy best first search" << endl;
    clock_t timer_start = clock();
    SparseStatePacker state_packer(task);
    StatePackerT packer(task);

    priority_queue<GBFSNode, vector<GBFSNode>> queue;

    SearchNode& root_node = space.insert_or_get_previous_node(packer.pack(task.initial_state), LiftedOperatorId::no_operator, StateID::no_state);
    this->heuristic_layer = heuristic.compute_heuristic(task.initial_state, task);
    root_node.open(0, this->heuristic_layer);
    cout << "Initial heuristic value " << this->heuristic_layer << endl;
    statistics.report_g_value_progress(0);
    queue.emplace(root_node.state_id, 0, this->heuristic_layer);

    if (task.is_goal(task.initial_state)) {
        print_goal_found(generator, timer_start);
        auto plan = space.extract_plan(root_node);
        cout << "Initial state is a goal" << endl;
        print_plan(plan, task);
        return utils::ExitCode::SUCCESS;
    }

    while (not queue.empty()) {
        GBFSNode gbfs_n = queue.top();
        StateID sid = gbfs_n.get_id();
        SearchNode node = space.get_node(sid);
        if (node.status == SearchNode::Status::CLOSED) {
            continue;
        }
        node.close();
        int h = node.h;
        int g = node.g;
        assert(g <= gbfs_n.g);
        queue.pop();
        statistics.report_g_value_progress(g);
        statistics.inc_expanded();

        if (this->g_layer < g) {
            this->g_layer = g;
        }
        if (h < this->heuristic_layer) {
            this->heuristic_layer = h;
            cout << "New heuristic value expanded: h=" << h
                 << " [this->state_counter: " << this->state_counter
                 << ", this->generations: " << this->generations
                 << ", time: " << double(clock() - timer_start) / CLOCKS_PER_SEC << "]" << '\n';
        }
        assert(sid.id() >= 0 && (unsigned) sid.id() < space.size());

        DBState state = packer.unpack(space.get_state(sid));

        if (task.is_goal(state)) {
            print_goal_found(generator, timer_start);
            auto plan = space.extract_plan(node);
            print_plan(plan, task);
            return utils::ExitCode::SUCCESS;
        }
        vector<LiftedOperatorId> applicable_actions = generator.get_applicable_actions(task.actions, state);

        this->generations += applicable_actions.size();
        statistics.inc_generated(applicable_actions.size());

        for (const LiftedOperatorId &op_id : applicable_actions) {
            const DBState &s = generator.generate_successors(op_id, task.actions[op_id.get_index()], state);
            const SparsePackedState packed = state_packer.pack(s);
            int dist = g + task.actions[op_id.get_index()].get_cost();
            int new_h = heuristic.compute_heuristic(s, task);
            statistics.inc_evaluations();
            auto& child_node = space.insert_or_get_previous_node(packer.pack(s), op_id, node.state_id);
            if (child_node.status == SearchNode::Status::NEW) {
                // Inserted for the first time in the map
                child_node.open(dist, new_h);
                queue.emplace(child_node.state_id, dist, new_h);
                this->state_counter++;
            }
            else {
                if (dist < child_node.g) {
                    child_node.open(dist, new_h); // Reopening
                    queue.emplace(child_node.state_id, dist, new_h);
                }
            }
        }
    }

    print_no_solution_found(timer_start);

    return utils::ExitCode::SEARCH_UNSOLVABLE;
}

template <class PackedStateT>
void GreedyBestFirstSearch<PackedStateT>::print_statistics() const {
    statistics.print_detailed_statistics();
//    space.print_statistics();
}

// explicit template instantiations
template class GreedyBestFirstSearch<SparsePackedState>;
template class GreedyBestFirstSearch<ExtensionalPackedState>;