#ifndef SEARCH_LAZY_BEST_FIRST_SEARCH_H
#define SEARCH_LAZY_BEST_FIRST_SEARCH_H


#include "search.h"
#include "search_space.h"
#include "../open_lists/greedy_open_list.h"

template <class PackedStateT>
class LazySearch : public SearchBase {

    int priority_preferred;
    int priority_regular;

    void boost_priority_preferred(int inc = 1000) {
        priority_preferred += inc;
    }

protected:
    SearchSpace<PackedStateT> space;

    int heuristic_layer{};
    bool all_operators_preferred;
    bool prune_relaxed_useless_operators;
public:
    explicit LazySearch(bool dual_queue, bool prune) :
        all_operators_preferred(dual_queue),
        prune_relaxed_useless_operators(prune)
    {
        priority_preferred = 0;
        priority_regular = 0;
    }

    using StatePackerT = typename PackedStateT::StatePackerT;

    utils::ExitCode search(const Task &task, SuccessorGenerator &generator, Heuristic &heuristic) override;

    void print_statistics() const override;

    StateID get_top_node(GreedyOpenList &preferred, GreedyOpenList &other) {
        if (priority_preferred >= priority_regular) {
            if (not preferred.empty()) {
                return preferred.remove_min();
            }
            else {
                return other.remove_min();
            }
        } else {
            if (not other.empty()) {
                return other.remove_min();
            }
            else {
              return preferred.remove_min();
            }
        }
    }
};


#endif  // SEARCH_LAZY_BEST_FIRST_SEARCH_H
