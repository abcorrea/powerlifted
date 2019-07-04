//
// Created by blaas on 04.07.19.
//

#ifndef SEARCH_GOALCOUNT_H
#define SEARCH_GOALCOUNT_H


#include "../state.h"
#include "../task.h"

class Goalcount {
public:
    int compute_heuristic(const State &s, const Task &task);
};


#endif //SEARCH_GOALCOUNT_H
