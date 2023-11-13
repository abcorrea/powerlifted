#ifndef SEARCH_SPARSE_STATES_H
#define SEARCH_SPARSE_STATES_H

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>
#include <boost/functional/hash/hash.hpp>

#include "../structures.h"

#include "../parallel_hashmap/phmap.h"
#include "../utils/hash.h"

/**
 * @brief The packed state representation is a more concise representation of states.
 * Each time we see a new atom, we give it a fresh index. To pack a state, we just compute the
 * indices of its atoms.
 */

class Task;
class DBState;

class SparseStatePacker;
class PackedStateHash;

class SparsePackedState {
public:
    using StatePackerT = SparseStatePacker;

    std::vector<std::vector<int>> packed_relations;
    std::vector<bool> nullary_atoms;


    bool operator==(const SparsePackedState &b) const;

    using HashT = PackedStateHash;

};

class PackedStateHash {
public:
    unsigned operator() (const SparsePackedState &s) const;
};


/**
 * @brief Pack and unpack states into a more compact representation
 */

class SparseStatePacker {
public:
    SparseStatePacker(const Task &task);

    SparsePackedState pack(const DBState &state);
    DBState unpack(const SparsePackedState &packed_state) const;

    std::vector<std::unordered_map<GroundAtom, int, utils::Hash<GroundAtom>>> atom_index;
    std::vector<std::unordered_map<int, GroundAtom>> index_to_atom;
    int next_idx;

private:
    long pack_tuple(const std::vector<int> &tuple, int predicate_index);

    std::vector<int> unpack_tuple(long tuple, int predicate_index) const;

};


#endif //SEARCH_SPARSE_STATES_H
