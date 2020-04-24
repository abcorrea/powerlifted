#ifndef SEARCH_GREEDY_BEST_FIRST_SEARCH_H
#define SEARCH_GREEDY_BEST_FIRST_SEARCH_H


#include "search.h"

template <class PackedStateT>
class GreedyBestFirstSearch : public SearchBase {
  public:
    int search(const Task &task, SuccessorGenerator *generator, Heuristic &heuristic) override;
};


#endif  // SEARCH_GREEDY_BEST_FIRST_SEARCH_H
