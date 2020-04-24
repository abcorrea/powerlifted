
#pragma once


#include "../states/sparse_states.h"
#include "../utils/segmented_vector.h"
#include "nodes.h"

class LiftedOperatorId;


template <typename StateT>
class SearchSpace {
protected:
    using StateHashT = typename StateT::HashT;
    segmented_vector::SegmentedVector<StateT> state_data;
    std::unordered_map<StateT, SearchNode, StateHashT> visited;

public:
    SearchSpace() : state_data(), visited() {}

    //! Return the number of registered states
    std::size_t size() const { return state_data.size(); }

    StateID create_initial_node(StateT&& state) {
        assert(state_data.size()==0);
        state_data.push_back(std::move(state));
        return StateID(state_data.size()-1);
    }

    SearchNode& insert_or_get_previous_node(StateT&& state, const LiftedOperatorId& op, StateID parent) {

    }
};

