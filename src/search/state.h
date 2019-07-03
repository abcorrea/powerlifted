#ifndef SEARCH_STATE_H
#define SEARCH_STATE_H

#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "structures.h"

/*
 * State is not very useful as a class, but we expect it to have further utility once we
 * start the search.
 *
 * The StaticInformation class right now is only syntatic sugar so we do not keep passing
 * static predicates in every single successor.
 */

class State {

public:
    std::vector<Relation> relations;

    explicit State(std::vector<Relation> relations) : relations(std::move(relations)) {
        // Explicit state constructor
    }

    State() = default;


    const std::vector<int> getObjects();

    void addTuple(int relation, const GroundAtom& args);
};

typedef State StaticInformation;


#endif //SEARCH_STATE_H
