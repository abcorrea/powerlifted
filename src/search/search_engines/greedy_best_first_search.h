#ifndef SEARCH_GREEDY_BEST_FIRST_SEARCH_H
#define SEARCH_GREEDY_BEST_FIRST_SEARCH_H


#include "search.h"
#include "search_space.h"

template <class PackedStateT>
class GreedyBestFirstSearch : public SearchBase {
protected:
    SearchSpace<PackedStateT> space;

    int heuristic_layer{};
public:
    using StatePackerT = typename PackedStateT::StatePackerT;

    utils::ExitCode search(const Task &task, SuccessorGenerator &generator, Heuristic &heuristic) override;

    void print_statistics() const override;
};


#endif  // SEARCH_GREEDY_BEST_FIRST_SEARCH_H
