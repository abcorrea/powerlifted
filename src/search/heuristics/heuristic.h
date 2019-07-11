#ifndef SEARCH_HEURISTIC_H
#define SEARCH_HEURISTIC_H


#include "../state.h"
#include "../task.h"

class Heuristic{
public:
    virtual int compute_heuristic(const State &s, const Task &task) = 0;
};


#endif //SEARCH_HEURISTIC_H
