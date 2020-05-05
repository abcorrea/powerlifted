#ifndef SEARCH_BREADTH_FIRST_SEARCH_H
#define SEARCH_BREADTH_FIRST_SEARCH_H

#include "search.h"
#include "search_space.h"

class DBState;

template <class PackedStateT>
class BreadthFirstSearch : public SearchBase {
protected:
    SearchSpace<PackedStateT> space;

public:
    using StatePackerT = typename PackedStateT::StatePackerT;

    int search(const Task &task, SuccessorGenerator &generator, Heuristic &heuristic) override;

    void print_statistics() const override;

    bool check_goal(const Task &task,
                    const SuccessorGenerator &generator,
                    clock_t timer_start,
                    const DBState &state,
                    const SearchNode &node) const;
};


#endif  // SEARCH_BREADTH_FIRST_SEARCH_H
