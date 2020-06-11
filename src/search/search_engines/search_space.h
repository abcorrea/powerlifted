
#pragma once

#include "../algorithms/int_hash_set.h"
#include "../utils/segmented_vector.h"
#include "nodes.h"

#include <fstream>
#include <unordered_set>

class LiftedOperatorId;


template <typename StateT>
class SearchSpace {
protected:
    using StateHashT = typename StateT::HashT;

    struct StateIDSemanticHash {
        const segmented_vector::SegmentedVector<StateT>& state_data;
        StateHashT hasher;

        explicit StateIDSemanticHash(const segmented_vector::SegmentedVector<StateT>& state_data)
            : state_data(state_data), hasher()
        {}

        std::size_t operator()(int id) const {
            return hasher(state_data[id]);
        }
    };

    struct StateIDSemanticEqual {
        const segmented_vector::SegmentedVector<StateT>& state_data;
        explicit StateIDSemanticEqual(const segmented_vector::SegmentedVector<StateT>& state_data)
            : state_data(state_data)
        {}

        bool operator()(int lhs, int rhs) const {
            assert(lhs >= 0 && (unsigned) lhs < state_data.size() &&
                   rhs >= 0 && (unsigned) rhs < state_data.size());
            return state_data[lhs] == state_data[rhs];
        }
    };

    using StateIDSet = int_hash_set::IntHashSet<StateIDSemanticHash, StateIDSemanticEqual>;

    segmented_vector::SegmentedVector<StateT> state_data;
    segmented_vector::SegmentedVector<SearchNode> node_data;
    StateIDSet registered_states;

public:
    SearchSpace() :
            state_data(),
            registered_states(StateIDSemanticHash(state_data), StateIDSemanticEqual(state_data))
    {}

    //! Return the number of registered states
    inline std::size_t size() const { return registered_states.size(); }

//    StateID create_initial_node(StateT&& state) {
//        assert(state_data.size()==0);
//        state_data.push_back(std::move(state));
//        return StateID(state_data.size()-1);
//    }

    SearchNode& insert_or_get_previous_node(StateT&& state, const LiftedOperatorId& op, StateID parent) {
        int id = state_data.size();
        state_data.push_back(std::move(state));
        auto result = registered_states.insert(id);

        if (result.second) { // It's an unseen state, create the node
            node_data.push_back(SearchNode(StateID(id), op, parent, 0));

        } else { // The state was already registered
            id = result.first;
            state_data.pop_back();
        }

        assert(registered_states.size() == static_cast<int>(state_data.size()));
        return node_data[id];
    }

    std::vector<LiftedOperatorId> extract_plan(const SearchNode& goal_node) const
    {
        std::vector<LiftedOperatorId> plan;
        SearchNode n = goal_node;

        for (;;) {
            if (n.op == LiftedOperatorId::no_operator) {  // We reached the root node
                assert(n.parent_state_id == StateID::no_state);
                break;
            }
            plan.push_back(n.op);
            n = node_data[n.parent_state_id.value];
        }
        reverse(plan.begin(), plan.end());
        return plan;
    }

    SearchNode &get_node(StateID id) {
        assert(id.value >= 0 && (unsigned) id.value < node_data.size());
        return node_data[id.value];
    }

    const StateT& get_state(StateID id) {
        assert(id.value >= 0 && (unsigned) id.value < state_data.size());
        return state_data[id.value];
    }

    void print_statistics() const {
        std::cout << "Number of registered states: " << size() << std::endl;
        registered_states.print_statistics();
    }
};

