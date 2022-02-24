
#include "search.h"
#include "search_space.h"
#include "utils.h"
#include "../task.h"
#include "../states/sparse_states.h"
#include "../states/extensional_states.h"

using namespace std;

bool SearchBase::is_useful_operator(const Task &task, const DBState &state,
                                    const vector<vector<GroundAtom>> &useful_atoms) {

    for (size_t pred_idx = 0; pred_idx < useful_atoms.size(); ++pred_idx) {
        if (task.predicates[pred_idx].isStaticPredicate()) continue;

        if (task.nullary_predicates.count(pred_idx) > 0) {
            // This is a bit of a hack. To save time, we only check if there is at least one tuple
            // considered useful for a nullary relation. As the relation is nullary, the only tuple
            // that can instantiate it is the empty one, so we know that if
            // useful_atoms[pred_idx].size() > 0, then this is the one instantiating it.
            if (useful_atoms[pred_idx].size() > 0 and state.get_nullary_atoms()[pred_idx])
                return true;
        }
        else {
            for (const auto &tuple: useful_atoms[pred_idx])
                if (state.get_tuples_of_relation(pred_idx).count(tuple) > 0)
                    return true;
        }
    }

    return false;
}

template<class PackedStateT>
bool SearchBase::check_goal(const Task &task,
                       const SuccessorGenerator &generator,
                       clock_t timer_start,
                       const DBState &state,
                       const SearchNode &node,
                       const SearchSpace<PackedStateT> &space) const {
    if (!task.is_goal(state)) return false;

    print_goal_found(generator, timer_start);
    auto plan = space.extract_plan(node);
    print_plan(plan, task);
    return true;
}

// explicit instantiations
template bool SearchBase::check_goal<SparsePackedState>(
        const Task &task, const SuccessorGenerator &generator, clock_t timer_start,
        const DBState &state, const SearchNode &node, const SearchSpace<SparsePackedState> &space) const;

template bool SearchBase::check_goal<ExtensionalPackedState>(
        const Task &task, const SuccessorGenerator &generator, clock_t timer_start,
        const DBState &state, const SearchNode &node, const SearchSpace<ExtensionalPackedState> &space) const;

