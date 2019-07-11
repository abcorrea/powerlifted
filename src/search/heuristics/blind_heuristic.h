#ifndef SEARCH_BLIND_HEURISTIC_H
#define SEARCH_BLIND_HEURISTIC_H

#include "heuristic.h"

class BlindHeuristic : public Heuristic {
public:
    int compute_heuristic(const State &s, const Task &task) override {
        return 1;
    }
};

#endif //SEARCH_BLIND_HEURISTIC_H
