#ifndef SEARCH_GREEDY_BEST_FIRST_SEARCH_H
#define SEARCH_GREEDY_BEST_FIRST_SEARCH_H


#include "search.h"
#include "search_space.h"

template <class PackedStateT>
class GreedyBestFirstSearch : public SearchBase {
protected:
    SearchSpace<PackedStateT> space;

    size_t state_counter{};

    int generations{};
    int g_layer{};
    int heuristic_layer{};
public:
    using StatePackerT = typename PackedStateT::StatePackerT;

    utils::ExitCode search(const Task &task, SuccessorGenerator &generator, Heuristic &heuristic) override;

    void print_statistics() const override;

};

class GBFSNode {
    StateID sid;
public:
    GBFSNode(StateID s, int g, int h) : sid(s), g(g), h(h) {}

    int g;
    int h;

    StateID get_id() {
        return sid;
    }

    bool operator==(const GBFSNode &other) const { return h == other.h and g == other.g; }
    bool operator!=(const GBFSNode &other) const { return !(*this == other); }
    bool operator<(const GBFSNode &other) const {
        if (h < other.h) return true;
        if ((h == other.h) and (g >= other.g)) return true;
        return false;
    }


};

#endif  // SEARCH_GREEDY_BEST_FIRST_SEARCH_H
