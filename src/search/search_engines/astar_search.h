#ifndef SEARCH_ASTAR_SEARCH_H
#define SEARCH_ASTAR_SEARCH_H

#include "search.h"

class AStarSearch: public Search {
public:
const std::vector<Action> &search(const Task &task,
                                  SuccessorGenerator generator,
                                  Heuristic &heuristic) const override;

};

#endif //SEARCH_ASTAR_SEARCH_H
