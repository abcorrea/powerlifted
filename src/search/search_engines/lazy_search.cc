#include "lazy_search.h"

#include "greedy_best_first_search.h"
#include "search.h"
#include "utils.h"
#include "../action.h"
#include "../task.h"
#include "../heuristics/heuristic.h"
#include "../open_lists/greedy_open_list.h"
#include "../states/extensional_states.h"
#include "../states/sparse_states.h"
#include "../successor_generators/successor_generator.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <queue>
#include <vector>

using namespace std;

template <class PackedStateT>
utils::ExitCode LazySearch<PackedStateT>::search(const Task &task,
                                                            SuccessorGenerator &generator,
                                                            Heuristic &heuristic)
{
    cout << "Starting greedy best first search" << endl;
    clock_t timer_start = clock();
    StatePackerT packer(task);

    GreedyOpenList preferred_queue;
    GreedyOpenList queue;

    SearchNode& root_node = space.insert_or_get_previous_node(packer.pack(task.initial_state), LiftedOperatorId::no_operator, StateID::no_state);
    heuristic_layer = heuristic.compute_heuristic(task.initial_state, task);
    root_node.open(0, heuristic_layer);
    if (heuristic_layer == numeric_limits<int>::max()) {
        cerr << "Initial state is unsolvable!" << endl;
        exit(1);
    }
    cout << "Initial heuristic value " << heuristic_layer << endl;
    statistics.report_f_value_progress(heuristic_layer);
    queue.do_insertion(root_node.state_id, make_pair(heuristic_layer, 0));

    if (check_goal(task, generator, timer_start, task.initial_state, root_node, space)) return utils::ExitCode::SUCCESS;

    while ((not queue.empty()) or (not preferred_queue.empty())) {
        StateID sid = get_top_node(preferred_queue, queue); //queue.remove_min();
        SearchNode &node = space.get_node(sid);
        DBState state = packer.unpack(space.get_state(sid));
        int h = heuristic.compute_heuristic(state, task);
        int g = node.g;
        node.update_h(h);
        if (h == std::numeric_limits<int>::max()) {
            statistics.inc_dead_ends();
            continue;
        }
        if (node.status == SearchNode::Status::CLOSED) {
            continue;
        }
        node.close();
        statistics.report_f_value_progress(h); // In GBFS f = h.
        statistics.inc_expanded();

        if (h < heuristic_layer) {
            heuristic_layer = h;
            cout << "New heuristic value expanded: h=" << h
                 << " [expansions: " << statistics.get_expanded()
                 << ", evaluations: " << statistics.get_evaluations()
                 << ", generations: " << statistics.get_generated()
                 << ", time: " << double(clock() - timer_start) / CLOCKS_PER_SEC << "]" << '\n';
        }
        assert(sid.id() >= 0 && (unsigned) sid.id() < space.size());

        if (check_goal(task, generator, timer_start, state, node, space)) return utils::ExitCode::SUCCESS;

        // Let's expand the state, one schema at a time. If necessary, i.e. if it really helps
        // performance, we could implement some form of std iterator
        for (const auto& action:task.actions) {
            auto applicable = generator.get_applicable_actions(action, state);
            statistics.inc_generated(applicable.size());

            for (const LiftedOperatorId& op_id:applicable) {
                DBState s = generator.generate_successor(op_id, action, state);
                int dist = g + action.get_cost();
                statistics.inc_evaluations();
                auto &child_node =
                    space.insert_or_get_previous_node(packer.pack(s), op_id, node.state_id);
                if (child_node.status==SearchNode::Status::NEW) {
                    // Inserted for the first time in the map
                    child_node.open(dist, h);
                    if (keep_relaxed_useless_operators or
                        is_useful_operator(s, heuristic.get_useful_atoms(), heuristic.get_useful_nullary_atoms())) {
                        preferred_queue.do_insertion(child_node.state_id, make_pair(h, dist));
                    }
                    else {
                        queue.do_insertion(child_node.state_id, make_pair(h, dist));
                    }
                } else {
                    if (dist < child_node.g) {
                        child_node.open(dist, h); // Reopening
                        statistics.inc_reopened();
                        if (keep_relaxed_useless_operators or
                            is_useful_operator(s, heuristic.get_useful_atoms(), heuristic.get_useful_nullary_atoms())) {
                            preferred_queue.do_insertion(child_node.state_id, make_pair(h, dist));
                        }
                        else {
                            queue.do_insertion(child_node.state_id, make_pair(h, dist));
                        }
                    }
                }
            }
        }
    }

    print_no_solution_found(timer_start);

    return utils::ExitCode::SEARCH_UNSOLVABLE;
}

template <class PackedStateT>
void LazySearch<PackedStateT>::print_statistics() const {
    statistics.print_detailed_statistics();
    space.print_statistics();
}

template<class PackedStateT>
bool LazySearch<PackedStateT>::is_useful_operator(const DBState &state,
                                                  const std::map<int, std::vector<GroundAtom>> &useful_atoms,
                                                  const std::vector<bool> &useful_nullary_atoms) {
    for (size_t j = 0; j < useful_nullary_atoms.size(); ++j) {
        if (useful_nullary_atoms[j] and state.get_nullary_atoms()[j]) {
            return true;
        }
    }
    for (auto &entry : useful_atoms) {
        size_t relation = entry.first;
        for (auto &tuple : entry.second) {
            if (state.get_tuples_of_relation(relation).count(tuple) > 0)
                return true;
        }
    }
    //cerr << "not useful" << endl;
    return false;
}

// explicit template instantiations
template class LazySearch<SparsePackedState>;
template class LazySearch<ExtensionalPackedState>;