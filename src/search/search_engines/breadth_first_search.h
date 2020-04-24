#ifndef SEARCH_BREADTH_FIRST_SEARCH_H
#define SEARCH_BREADTH_FIRST_SEARCH_H

#include "search.h"

template <class PackedStateT>
class BreadthFirstSearch : public SearchBase {
  public:
      using StatePackerT = typename PackedStateT::StatePackerT;

    int search(const Task &task, SuccessorGenerator *generator, Heuristic &heuristic) override;
};


#endif  // SEARCH_BREADTH_FIRST_SEARCH_H
