
#include "search.h"
#include "search_space.h"
#include "utils.h"
#include "../task.h"
#include "../states/sparse_states.h"
#include "../states/extensional_states.h"

using namespace std;

bool SearchBase::is_useful_operator(const Task &task, const DBState &state,
                                    const map<int, std::vector<GroundAtom>> &useful_atoms,
                                    const vector<bool> &useful_nullary_atoms) {
    for (size_t j = 0; j < useful_nullary_atoms.size(); ++j) {
        if (useful_nullary_atoms[j] and state.get_nullary_atoms()[j]) {
            return true;
        }
    }
    for (auto &entry : useful_atoms) {
        size_t relation = entry.first;
        if (task.predicates[relation].isStaticPredicate())
            continue;
        for (auto &tuple : entry.second) {
            if (state.get_tuples_of_relation(relation).count(tuple) > 0)
                return true;
        }
    }
    //cerr << "not useful" << endl;
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

