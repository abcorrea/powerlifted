#ifndef SEARCH_BREADTH_FIRST_SEARCH_H
#define SEARCH_BREADTH_FIRST_SEARCH_H


#include "search.h"
#include "search_space.h"

template <class PackedStateT>
class BreadthFirstSearch : public SearchBase {
protected:
    SearchSpace<PackedStateT> space;

public:
    using StatePackerT = typename PackedStateT::StatePackerT;

    utils::ExitCode search(const Task &task, SuccessorGenerator &generator, Heuristic &heuristic) override;

    void print_statistics() const override;
};


#endif  // SEARCH_BREADTH_FIRST_SEARCH_H
