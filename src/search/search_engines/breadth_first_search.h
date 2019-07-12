#ifndef SEARCH_BREADTH_FIRST_SEARCH_H
#define SEARCH_BREADTH_FIRST_SEARCH_H

#include "search.h"

class BreadthFirstSearch: public Search {
public:
    const std::vector<Action> &search(const Task &task,
                                      SuccessorGenerator generator,
                                      Heuristic &heuristic) const override;
    std::vector<Action> plan;
};


#endif //SEARCH_BREADTH_FIRST_SEARCH_H
