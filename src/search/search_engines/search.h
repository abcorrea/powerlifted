#ifndef SEARCH_SEARCH_H
#define SEARCH_SEARCH_H

#include "../action_schema.h"
#include "../action.h"
#include "../states/state.h"
#include "../states/sparse_states.h"
#include "../structures.h"
#include "../task.h"

#include "../heuristics/heuristic.h"
#include "../successor_generators/successor_generator.h"
#include "../utils/segmented_vector.h"

#include <utility>
#include <vector>

#define SOLVED 0
#define NOT_SOLVED 1
#define DEBUG_GRACEFUL_EXIT 0

class Node {
public:
    Node(int g, int h, size_t id) : g(g), h(h), id(id) {}

    int g;
    int h;
    size_t id;
};

struct NodeComparison {
    bool operator()(const Node &n, const Node &m) const {
        if (n.h!=m.h) return n.h > m.h;
        else return n.g > m.g;
    }
};

class SearchBase {
public:
    SearchBase() = default;
    virtual ~SearchBase() = default;

    virtual int search(const Task &task,
                       SuccessorGenerator *generator,
                       Heuristic &heuristic) = 0;
};

template<class PackedStateT>
class Search : public SearchBase {

public:
    Search() = default;
    ~Search() override = default;

    static void extract_plan(
            segmented_vector::SegmentedVector<pair<int, Action>> &cheapest_parent,
            SparsePackedState state,
            unordered_map<SparsePackedState, int, PackedStateHash> &visited,
            segmented_vector::SegmentedVector<SparsePackedState> &index_to_state,
            const SparseStatePacker &packer, const Task &task);

    std::vector<Action> plan;
    void print_no_solution_found(clock_t timer_start) const;
    void print_goal_found(
        const Task &task,
        const SuccessorGenerator *generator,
        clock_t timer_start,
        const SparseStatePacker &state_packer,
        int generations_until_last_jump,
        segmented_vector::SegmentedVector<pair<int, Action>> &cheapest_parent,
        segmented_vector::SegmentedVector<SparsePackedState> &index_to_state,
        unordered_map<SparsePackedState, int, PackedStateHash> &visited,
        const State &state) const;

protected:
    size_t state_counter{};
    int generations{};
    int generations_last_jump{};
    int g_layer{};
    int heuristic_layer{};
};

#endif //SEARCH_SEARCH_H
