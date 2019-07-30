#include <iostream>
#include <cassert>
#include <vector>
#include <queue>

#include "successor_generator.h"
#include "../action.h"

using namespace std;

const GroundAtom &SuccessorGenerator::tuple_to_atom(const vector<int> &tuple,
                                                    const vector<int> &indices,
                                                    const Atom &eff) {


    /*
     *    This action generates the ground atom produced by an atomic effect given an instantiation of
     *    its parameters.
     *
     *    First, we rearrange the indices. Then, we create the atom based on whether the argument
     *    is a constant or not. If it is, then we simply pass the constant value; otherwise we use
     *    the instantiation that we found.
     */

    vector<int> ordered_tuple(tuple.size(), -1);
    assert (tuple.size() == indices.size());
    for (int i = 0; i < indices.size(); ++i) {
        ordered_tuple[indices[i]] = tuple[i];
    }

    ground_atom.clear();
    ground_atom.reserve(eff.tuples.size());
    for (int i = 0; i < eff.tuples.size(); i++) {
        if (!eff.tuples[i].constant)
            ground_atom.push_back(ordered_tuple[eff.tuples[i].index]);
        else
            ground_atom.push_back(eff.tuples[i].index);
    }

    //Sanity check
    for (int v : ground_atom) {
        assert (v != -1);
    }

    return ground_atom;
}


