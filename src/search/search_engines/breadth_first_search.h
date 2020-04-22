#ifndef SEARCH_BREADTH_FIRST_SEARCH_H
#define SEARCH_BREADTH_FIRST_SEARCH_H

#include "search.h"

template<class PackedStateT>
class BreadthFirstSearch: public Search<PackedStateT> {
public:
    int search(const Task &task,
                     SuccessorGenerator *generator,
                     Heuristic &heuristic) override;

    using Search<PackedStateT>::print_goal_found;
};


#endif //SEARCH_BREADTH_FIRST_SEARCH_H
