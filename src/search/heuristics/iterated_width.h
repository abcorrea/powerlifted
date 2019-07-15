#ifndef SEARCH_ITERATED_WIDTH_H
#define SEARCH_ITERATED_WIDTH_H

#include "heuristic.h"

struct IWVectorHash {
    size_t operator()(const std::vector<int>& v) const {
        std::hash<int> hasher;
        size_t seed = 0;
        for (int i : v) {
            seed ^= hasher(i) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        }
        return seed;
    }
};

class IteratedWidth : public Heuristic {
    /*
     * Implements IW1 evaluator
     */
public:
    int compute_heuristic(const State &s, const Task &task) override;
private:
    std::vector<std::unordered_set<std::vector<int>, IWVectorHash>> history;
    bool first_time = true;


};


#endif //SEARCH_ITERATED_WIDTH_H
