#ifndef SEARCH_GREEDY_BEST_FIRST_SEARCH_H
#define SEARCH_GREEDY_BEST_FIRST_SEARCH_H


#include "search.h"

class GreedyBestFirstSearch : public Search {
public:
    int search(const Task &task,
                     SuccessorGenerator *generator,
                     Heuristic &heuristic) const override;
    std::vector<Action> plan;
};


#endif //SEARCH_GREEDY_BEST_FIRST_SEARCH_H
