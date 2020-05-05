
#include "breadth_first_search.h"
#include "../states/extensional_states.h"
#include "../states/sparse_states.h"
#include "../successor_generators/successor_generator.h"
#include "../task.h"
#include "utils.h"
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
int BreadthFirstSearch<PackedStateT>::search(const Task &task,
                                             SuccessorGenerator &generator,
                                             Heuristic &heuristic)
{
    cout << "Starting breadth first search" << endl;
    clock_t timer_start = clock();

    StatePackerT packer(task);
    std::queue<StateID> queue;

    SearchNode& root_node = space.insert_or_get_previous_node(packer.pack(task.initial_state), LiftedOperatorId::no_operator, StateID::no_state);
    root_node.open(0);
    statistics.report_f_value_progress(root_node.g);
    queue.emplace(root_node.state_id);

    if (check_goal(task, generator, timer_start, task.initial_state, root_node)) return SOLVED;

    while (not queue.empty()) {
        StateID sid = queue.front();
        queue.pop();
        SearchNode node = space.get_node(sid);
        if (node.status == SearchNode::Status::CLOSED) {
            continue;
        }
        node.close();
        statistics.report_f_value_progress(node.g);
        statistics.inc_expanded();

        assert(sid.id() >= 0 && (unsigned) sid.id() < space.size());

        DBState state = packer.unpack(space.get_state(sid));
        vector<LiftedOperatorId> applicable_actions = generator.get_applicable_actions(task.actions, state);

        statistics.inc_generated(applicable_actions.size());

        for (const LiftedOperatorId &a : applicable_actions) {
            const DBState &s = generator.generate_successors(a, task.actions[a.get_index()], state);
            auto& child_node = space.insert_or_get_previous_node(packer.pack(s), a, node.state_id);
            if (child_node.status == SearchNode::Status::NEW) {
                child_node.open(node.g+1);

                if (check_goal(task, generator, timer_start, s, child_node)) return SOLVED;

                queue.emplace(child_node.state_id);
            }
        }
    }

    print_no_solution_found(timer_start);

    return NOT_SOLVED;
}

template <class PackedStateT>
void BreadthFirstSearch<PackedStateT>::print_statistics() const {
    statistics.print_detailed_statistics();
    space.print_statistics();
}

// explicit template instantiations
template class BreadthFirstSearch<SparsePackedState>;
template class BreadthFirstSearch<ExtensionalPackedState>;