#ifndef SEARCH_SEARCH_ENGINES_BREADTH_FIRST_WIDTH_SEARCH_H_
#define SEARCH_SEARCH_ENGINES_BREADTH_FIRST_WIDTH_SEARCH_H_

#include "search.h"
#include "search_space.h"
#include "../novelty/atom_counter.h"

template <class PackedStateT>
class BreadthFirstWidthSearch : public SearchBase {
    int width;
    bool prune_states;

protected:
    SearchSpace<PackedStateT> space;

    int heuristic_layer{};

    AtomCounter initialize_counter_with_gc(const Task &task);

public:
    explicit BreadthFirstWidthSearch(int width) : width(width), prune_states(false) {}

    explicit BreadthFirstWidthSearch(int width, bool prune) : width(width), prune_states(prune) {}

    using StatePackerT = typename PackedStateT::StatePackerT;

    utils::ExitCode search(const Task &task, SuccessorGenerator &generator, Heuristic &heuristic) override;

    void print_statistics() const override;
};

#endif //SEARCH_SEARCH_ENGINES_BREADTH_FIRST_WIDTH_SEARCH_H_
