#ifndef SEARCH_SEARCH_ENGINES_BREADTH_FIRST_WIDTH_SEARCH_H_
#define SEARCH_SEARCH_ENGINES_BREADTH_FIRST_WIDTH_SEARCH_H_

#include "search.h"
#include "search_space.h"

#include "../novelty/atom_counter.h"
#include "../novelty/node_novelty.h"
#include "../novelty/standard_novelty.h"

#include "../options.h"

template <class PackedStateT>
class BreadthFirstWidthSearch : public SearchBase {
    AtomCounter atom_counter;
    int width;
    int method;
    bool prune_states;
    bool only_effects_opt;
    bool early_stop;

protected:
    SearchSpace<PackedStateT> space;

    int heuristic_layer{};

    AtomCounter initialize_counter_with_gc(const Task &task);

public:

    explicit BreadthFirstWidthSearch(int width, const Options &opt, int method);

    using StatePackerT = typename PackedStateT::StatePackerT;

    utils::ExitCode search(const Task &task, SuccessorGenerator &generator, Heuristic &heuristic) override;

    void print_statistics() const override;
    AtomCounter initialize_counter_with_useful_atoms(const Task &task);
};

#endif //SEARCH_SEARCH_ENGINES_BREADTH_FIRST_WIDTH_SEARCH_H_
