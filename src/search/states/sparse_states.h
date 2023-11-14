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

    std::vector<int> packed_relations;
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

    phmap::flat_hash_map<std::pair<int, GroundAtom>, int, utils::Hash<std::pair<int, GroundAtom>>> atom_index;
    phmap::flat_hash_map<int, std::pair<int, GroundAtom>> index_to_atom;
    int next_idx;

private:
    int pack_tuple(const std::vector<int> &tuple, int predicate_index);

    std::pair<int, GroundAtom> unpack_tuple(int index) const;

    int num_predicates;

};


#endif //SEARCH_SPARSE_STATES_H
