#include "breadth_first_width_search.h"
#include "search.h"
#include "utils.h"

#include "../action.h"

#include "../heuristics/heuristic.h"
#include "../open_lists/tiebreaking_open_list.h"
#include "../states/extensional_states.h"
#include "../states/sparse_states.h"
#include "../successor_generators/successor_generator.h"
#include "../utils/timer.h"
#include "../novelty/standard_novelty.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <vector>

using namespace std;

template <class PackedStateT>
utils::ExitCode BreadthFirstWidthSearch<PackedStateT>::search(const Task &task,
                                                            SuccessorGenerator &generator,
                                                            Heuristic &heuristic)
{
    cout << "Starting BFWS" << endl;
    clock_t timer_start = clock();
    StatePackerT packer(task);

    Goalcount gc;

    // We use a GreedyOpenList (ordered by the novelty value) for now. This is done to make the
    // search algorithm complete.
    TieBreakingOpenList queue;

    SearchNode& root_node = space.insert_or_get_previous_node(packer.pack(task.initial_state),
        LiftedOperatorId::no_operator, StateID::no_state);
    utils::Timer t;

    StandardNovelty novelty_evaluator(task);
    root_node.open(0, 1);

    statistics.inc_evaluations();
    cout << "Initial heuristic value " << heuristic_layer << endl;
    statistics.report_f_value_progress(heuristic_layer);
    queue.do_insertion(root_node.state_id, {1, gc.compute_heuristic(task.initial_state, task), 0});

    if (check_goal(task, generator, timer_start, task.initial_state, root_node, space)) return utils::ExitCode::SUCCESS;

    while (not queue.empty()) {
        StateID sid = queue.remove_min();
        SearchNode &node = space.get_node(sid);
        int g = node.g;
        if (node.status == SearchNode::Status::CLOSED) {
            continue;
        }
        node.close();
        statistics.report_f_value_progress(g); // In GBFS f = h.
        statistics.inc_expanded();

        assert(sid.id() >= 0 && (unsigned) sid.id() < space.size());

        DBState state = packer.unpack(space.get_state(sid));
        if (check_goal(task, generator, timer_start, state, node, space)) return utils::ExitCode::SUCCESS;

        for (const auto& action:task.actions) {
            auto applicable = generator.get_applicable_actions(action, state);
            statistics.inc_generated(applicable.size());

            for (const LiftedOperatorId& op_id:applicable) {
                DBState s = generator.generate_successor(op_id, action, state);
                int dist = g + action.get_cost();
                int novelty_value;
                int unsatisfied_goals = gc.compute_heuristic(s, task);
                if (width == 1)
                    novelty_value = novelty_evaluator.compute_novelty_k1(task, s, unsatisfied_goals);
                else
                    novelty_value = novelty_evaluator.compute_novelty_k2(task, s, unsatisfied_goals);
                statistics.inc_evaluations();
                statistics.inc_evaluated_states();
                if ((prune_states) and (novelty_value == StandardNovelty::NOVELTY_GREATER_THAN_TWO))
                    continue;
                auto& child_node = space.insert_or_get_previous_node(packer.pack(s), op_id, node.state_id);
                if (child_node.status == SearchNode::Status::NEW) {
                    child_node.open(dist, novelty_value);
                    if (check_goal(task, generator, timer_start, s, child_node, space)) return utils::ExitCode::SUCCESS;
                    queue.do_insertion(child_node.state_id, {novelty_value, unsatisfied_goals, dist});
                }
            }
        }
    }

    print_no_solution_found(timer_start);

    if (prune_states)
        return utils::ExitCode::SEARCH_UNSOLVED_INCOMPLETE;
    return utils::ExitCode::SEARCH_UNSOLVABLE;
}

template <class PackedStateT>
void BreadthFirstWidthSearch<PackedStateT>::print_statistics() const {
    statistics.print_detailed_statistics();
    space.print_statistics();
}

// explicit template instantiations
template class BreadthFirstWidthSearch<SparsePackedState>;
template class BreadthFirstWidthSearch<ExtensionalPackedState>;//