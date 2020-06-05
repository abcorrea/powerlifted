
#include "breadth_first_search.h"

#include "utils.h"

#include "../states/extensional_states.h"
#include "../states/sparse_states.h"
#include "../successor_generators/successor_generator.h"
#include "../task.h"

#include <iostream>
#include <queue>
#include <vector>

using namespace std;

template <class PackedStateT>
bool BreadthFirstSearch<PackedStateT>::check_goal(
    const Task &task,
    const SuccessorGenerator &generator,
    clock_t timer_start,
    const DBState &state,
    const SearchNode &node) const
{
    if (!task.is_goal(state)) return false;

    print_goal_found(generator, timer_start);
    auto plan = space.extract_plan(node);
    print_plan(plan, task);
    return true;
}

template <class PackedStateT>
utils::ExitCode BreadthFirstSearch<PackedStateT>::search(const Task &task,
                                             SuccessorGenerator &generator,
                                             Heuristic &heuristic)
{
    cout << "Starting breadth first search" << endl;
    clock_t timer_start = clock();

    StatePackerT packer(task);
    std::queue<StateID> queue;

    SearchNode& root_node = space.insert_or_get_previous_node(packer.pack(task.initial_state), LiftedOperatorId::no_operator, StateID::no_state);
    root_node.open(0);
    statistics.report_f_value_progress(root_node.f);
    queue.emplace(root_node.state_id);

    if (check_goal(task, generator, timer_start, task.initial_state, root_node)) return utils::ExitCode::SUCCESS;

    while (not queue.empty()) {
        StateID sid = queue.front();
        queue.pop();
        SearchNode node = space.get_node(sid);
        if (node.status == SearchNode::Status::CLOSED) {
            continue;
        }
        node.close();
        statistics.report_f_value_progress(node.f);
        statistics.inc_expanded();

        assert(sid.id() >= 0 && (unsigned) sid.id() < space.size());

        DBState state = packer.unpack(space.get_state(sid));

        // Let's expand the state, one schema at a time. If necessary, i.e. if it really helps
        // performance, we could implement some form of std iterator
        for (std::size_t aidx = 0, sz = task.actions.size(); aidx < sz; ++aidx) {
            const auto& action = task.actions[aidx];
            vector<LiftedOperatorId> applicable_actions;
            generator.get_applicable_actions(action, state, applicable_actions);
            statistics.inc_generated(applicable_actions.size());

            for (const LiftedOperatorId &op_id : applicable_actions) {
                assert((std::size_t) op_id.get_index() == aidx);
                const DBState &s = generator.generate_successors(op_id, action, state);
                auto& child_node = space.insert_or_get_previous_node(packer.pack(s), op_id, node.state_id);
                if (child_node.status == SearchNode::Status::NEW) {
                    child_node.open(node.f+1);

                    if (check_goal(task, generator, timer_start, s, child_node)) return utils::ExitCode::SUCCESS;

                    queue.emplace(child_node.state_id);
                }
            }
        }
    }

    print_no_solution_found(timer_start);

    return utils::ExitCode::SEARCH_UNSOLVABLE;
}

template <class PackedStateT>
void BreadthFirstSearch<PackedStateT>::print_statistics() const {
    statistics.print_detailed_statistics();
    space.print_statistics();
}

// explicit template instantiations
template class BreadthFirstSearch<SparsePackedState>;
template class BreadthFirstSearch<ExtensionalPackedState>;
