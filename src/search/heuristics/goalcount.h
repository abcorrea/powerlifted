#ifndef SEARCH_GOALCOUNT_H
#define SEARCH_GOALCOUNT_H


#include "../state.h"
#include "../task.h"
#include "heuristic.h"

/**
 * @brief Compute hamming distance between goal condition and state s.
 *
 * @note Goal-aware. Inadmissible.
 *
 */
class Goalcount : public Heuristic {
public:
    int compute_heuristic(const State &s, const Task &task) final;
};


#endif //SEARCH_GOALCOUNT_H
