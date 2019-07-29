#ifndef SEARCH_ITERATED_WIDTH_H
#define SEARCH_ITERATED_WIDTH_H

#include "heuristic.h"
#include "goalcount.h"

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
