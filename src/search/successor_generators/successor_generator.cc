#include <iostream>
#include <cassert>
#include <vector>
#include <queue>

#include "successor_generator.h"
#include "../action.h"

using namespace std;

GroundAtom SuccessorGenerator::tuple_to_atom(const vector<int> &tuple, const vector<int> &indices, const Atom &eff) {
    vector<int> ordered_tuple(tuple.size(), -1);
    assert (tuple.size() == indices.size());
    for (int i = 0; i < indices.size(); ++i) {
        ordered_tuple[indices[i]] = tuple[i];
    }

    GroundAtom ground_atom(eff.tuples.size(), -1);
    for (int i = 0; i < ground_atom.size(); i++) {
        ground_atom[i] = ordered_tuple[eff.tuples[i].index];
    }

    //Sanity check
    for (int v : ground_atom) {
        assert (v != -1);
    }

    return ground_atom;
}


