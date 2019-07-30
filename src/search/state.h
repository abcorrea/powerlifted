#ifndef SEARCH_STATE_H
#define SEARCH_STATE_H

#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/functional/hash.hpp>

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
    std::vector<bool> nullary_atoms;

    explicit State(std::vector<Relation> relations,
                   std::vector<bool> nullary_atoms) : relations(std::move(relations)),
                                                      nullary_atoms(std::move(nullary_atoms)) {
        // Explicit state constructor
    }

    State() = default;

    const std::vector<int> getObjects();

    void addTuple(int relation, const GroundAtom& args);

    bool operator==(const State &other) const {
        return relations == other.relations;
    }

    friend std::size_t hash_value(const State &s) {
        std::size_t seed = 0;
        for (bool b : s.nullary_atoms) {
            boost::hash_combine(seed, b);
        }
        for (const Relation &r : s.relations) {
            for (const GroundAtom &vga : r.tuples) {
                boost::hash_combine(seed, vga);
            }
        }
        return seed;
    }
};

typedef State StaticInformation;


#endif //SEARCH_STATE_H
