#ifndef SEARCH_SEARCH_ENGINES_BREADTH_FIRST_WIDTH_SEARCH_H_
#define SEARCH_SEARCH_ENGINES_BREADTH_FIRST_WIDTH_SEARCH_H_

#include "search.h"
#include "search_space.h"

template <class PackedStateT>
class BreadthFirstWidthSearch : public SearchBase {
protected:
    SearchSpace<PackedStateT> space;

    int heuristic_layer{};
public:
    using StatePackerT = typename PackedStateT::StatePackerT;

    utils::ExitCode search(const Task &task, SuccessorGenerator &generator, Heuristic &heuristic) override;

    void print_statistics() const override;
};

#endif //SEARCH_SEARCH_ENGINES_BREADTH_FIRST_WIDTH_SEARCH_H_
