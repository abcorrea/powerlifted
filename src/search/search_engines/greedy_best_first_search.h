#ifndef SEARCH_GREEDY_BEST_FIRST_SEARCH_H
#define SEARCH_GREEDY_BEST_FIRST_SEARCH_H


#include "search.h"

template<class PackedStateT>
class GreedyBestFirstSearch : public Search<PackedStateT> {
public:
    int search(const Task &task,
                     SuccessorGenerator *generator,
                     Heuristic &heuristic) override;

    using Search<PackedStateT>::print_goal_found;
};


#endif //SEARCH_GREEDY_BEST_FIRST_SEARCH_H
