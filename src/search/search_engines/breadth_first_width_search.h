#ifndef SEARCH_SEARCH_ENGINES_BREADTH_FIRST_WIDTH_SEARCH_H_
#define SEARCH_SEARCH_ENGINES_BREADTH_FIRST_WIDTH_SEARCH_H_

#include "search.h"
#include "search_space.h"

#include "../novelty/atom_counter.h"
#include "../novelty/standard_novelty.h"

#include "../options.h"

template <class PackedStateT>
class BreadthFirstWidthSearch : public SearchBase {
    AtomCounter atom_counter;
    int width;
    bool prune_states;
    int method;
    std::string datalog_file_name;

protected:
    SearchSpace<PackedStateT> space;

    int heuristic_layer{};

    AtomCounter initialize_counter_with_gc(const Task &task);

public:
    explicit BreadthFirstWidthSearch(int width) : width(width), prune_states(false), method(StandardNovelty::R_0) {}

    explicit BreadthFirstWidthSearch(int width, bool prune) : width(width), prune_states(prune), method(StandardNovelty::R_0) {}

    explicit BreadthFirstWidthSearch(int width, const Options &opt) : width(width), prune_states(false), method(StandardNovelty::R_X) {
        std::cout << "Using version with R-X" << std::endl;
        datalog_file_name = opt.get_datalog_file();
    }

    using StatePackerT = typename PackedStateT::StatePackerT;

    utils::ExitCode search(const Task &task, SuccessorGenerator &generator, Heuristic &heuristic) override;

    void print_statistics() const override;
    AtomCounter initialize_counter_with_useful_atoms(const Task &task);
};

#endif //SEARCH_SEARCH_ENGINES_BREADTH_FIRST_WIDTH_SEARCH_H_
