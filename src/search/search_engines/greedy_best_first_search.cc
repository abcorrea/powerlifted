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

    priority_queue<GBFSNode> queue;

    SearchNode& root_node = space.insert_or_get_previous_node(packer.pack(task.initial_state), LiftedOperatorId::no_operator, StateID::no_state);
    heuristic_layer = heuristic.compute_heuristic(task.initial_state, task);
    root_node.open(0, heuristic_layer);
    cout << "Initial heuristic value " << heuristic_layer << endl;
    statistics.report_f_value_progress(heuristic_layer);
    queue.emplace(root_node.state_id, 0, heuristic_layer);

    if (check_goal(task, generator, timer_start, task.initial_state, root_node, space)) return utils::ExitCode::SUCCESS;

    while (not queue.empty()) {
        const GBFSNode gbfs_n = queue.top();
        StateID sid = gbfs_n.get_id();
        SearchNode &node = space.get_node(sid);
        int h = node.h;
        int g = node.g;
        queue.pop();
        if (node.status == SearchNode::Status::CLOSED) {
            continue;
        }
        node.close();
        assert(g <= gbfs_n.g);
        statistics.report_f_value_progress(h); // In GBFS f = h.
        statistics.inc_expanded();

        if (this->g_layer < g) {
            this->g_layer = g;
        }
        if (h < this->heuristic_layer) {
            this->heuristic_layer = h;
            cout << "New heuristic value expanded: h=" << h
                 << " [state_counter: " << state_counter
                 << ", generations: " << generations
                 << ", time: " << double(clock() - timer_start) / CLOCKS_PER_SEC << "]" << '\n';
        }
        assert(sid.id() >= 0 && (unsigned) sid.id() < space.size());

        DBState state = packer.unpack(space.get_state(sid));
        if (check_goal(task, generator, timer_start, state, node, space)) return utils::ExitCode::SUCCESS;

        vector<LiftedOperatorId> applicable_actions = generator.get_applicable_actions(task.actions, state);

        generations += applicable_actions.size();
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
                    statistics.inc_reopened();
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