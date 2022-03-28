#ifndef SEARCH_ASTAR_SEARCH_H
#define SEARCH_ASTAR_SEARCH_H

#include "search.h"
#include "search_space.h"

template <class PackedStateT>
class AStarSearch : public SearchBase {
protected:
    SearchSpace<PackedStateT> space;

    int heuristic_layer{};
public:
    using StatePackerT = typename PackedStateT::StatePackerT;

    utils::ExitCode search(const Task &task, SuccessorGenerator &generator, Heuristic &heuristic) override;

    void print_statistics() const override;
};


#endif
