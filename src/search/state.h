#ifndef SEARCH_STATE_H
#define SEARCH_STATE_H

#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "structures.h"

class State {

public:
    std::vector<Relation> relations;

    explicit State(std::vector<Relation> relations) : relations(std::move(relations)) {
        // Explicit state constructor
    }

    State() = default;

    const std::vector<Relation> &getRelations() const {
        return relations;
    }

    const std::vector<int> getObjects();

    void addTuple(int relation, const GroundAtom& args);
};


#endif //SEARCH_STATE_H
