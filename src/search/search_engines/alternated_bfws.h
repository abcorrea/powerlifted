#ifndef SEARCH_SEARCH_ENGINES_ALTERNATED_BFWS_H_
#define SEARCH_SEARCH_ENGINES_ALTERNATED_BFWS_H_

#include "search.h"
#include "search_space.h"

#include "../heuristics/ff_heuristic.h"

#include "../novelty/atom_counter.h"
#include "../novelty/node_novelty.h"
#include "../novelty/standard_novelty.h"

#include "../open_lists/tiebreaking_open_list.h"
#include "../open_lists/greedy_open_list.h"

#include "../options.h"

template <class PackedStateT>
class AlternatedBFWS : public SearchBase {
    AtomCounter atom_counter;
    int width;
    bool only_effects_opt;

    std::string heuristic_type;

protected:
    SearchSpace<PackedStateT> space;

    int heuristic_layer{};

    AtomCounter initialize_counter_with_gc(const Task &task);

public:
    explicit AlternatedBFWS(int width, const Options &opt) : width(width) {
        std::cout << "Using Dual-Queue BFWS" << std::endl;
        // By default we use h-add as heuristic, unless explicitly asked to use FF
        heuristic_type = opt.get_evaluator();
    }

    using StatePackerT = typename PackedStateT::StatePackerT;

    utils::ExitCode search(const Task &task, SuccessorGenerator &generator, Heuristic &heuristic) override;
    void print_statistics() const override;
    AtomCounter initialize_counter_with_useful_atoms(const Task &task, Heuristic &delete_free_h) const;
};


#endif //SEARCH_SEARCH_ENGINES_ALTERNATED_BFWS_H_
