#ifndef SEARCH_GREEDY_BEST_FIRST_SEARCH_H
#define SEARCH_GREEDY_BEST_FIRST_SEARCH_H


#include "search.h"

template <class PackedStateT>
class GreedyBestFirstSearch : public SearchBase {
  public:
    utils::ExitCode search(const Task &task, SuccessorGenerator &generator, Heuristic &heuristic) override;

    void print_statistics() const override;

protected:
    size_t state_counter{};

    int generations{};
    int g_layer{};
    int heuristic_layer{};
};


#endif  // SEARCH_GREEDY_BEST_FIRST_SEARCH_H
