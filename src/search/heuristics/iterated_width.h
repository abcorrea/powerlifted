#ifndef SEARCH_ITERATED_WIDTH_H
#define SEARCH_ITERATED_WIDTH_H

#include "heuristic.h"
#include "goalcount.h"

/**
 * @brief If there is a tuple which appeared in a state with goalcount higher than the current one, return 1.
 * Otherwise, return 2.  If it is a goal state, return 0.
 *
 * @var history: Keep track of all tuples and the lowest goalcount value in which they appeared.
 *
 * @note Goal-aware. Inadmissible.
 *
 * @todo change to regular iw1
 */
class IteratedWidth : public Heuristic {
    /*
     * Implements IW1 evaluator
     */
public:
    int compute_heuristic(const State &s, const Task &task) final;
private:
    std::vector<std::unordered_map<std::vector<int>, int, TupleHash>> history;
    bool first_time = true;
    Goalcount goalcount;


};


#endif //SEARCH_ITERATED_WIDTH_H
