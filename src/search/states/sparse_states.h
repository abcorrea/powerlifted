#ifndef SEARCH_SPARSE_STATES_H
#define SEARCH_SPARSE_STATES_H

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>
#include <unordered_map>

#include <boost/functional/hash/hash.hpp>

/**
 * @brief The packed state representation is a more concise representation of states,
 * based on the Fast Downward source code.
 *
 * @details We represent a state as a vector of relations and a vector of
 * nullary atoms. Each relation is a set of tuples, which can be interpreted as a 'table'.
 * In order to make the representation more concise, we first order the tuples in the
 * sets corresponding to each relation in a deterministic way. We then hash these sets
 * in a well-defined order (by the predicate symbol of the corresponding relation).
 * Last, we combine all these hash values together and also combine with a hash over
 * the predicate symbols and the truth value of the nullary relations
 * (i.e., predicates) of the state.
 * This packed state representation is loosely based on the PDB storage system used
 * by Fast Downward.
 *
 */

class Task;
class DBState;

class SparseStatePacker;
class PackedStateHash;

class SparsePackedState {
public:
    using StatePackerT = SparseStatePacker;

    std::vector<std::vector<long>> packed_relations;
    std::vector<int> predicate_symbols;
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

    SparsePackedState pack(const DBState &state) const;

    DBState unpack(const SparsePackedState &packed_state) const;

private:
    long pack_tuple(const std::vector<int> &tuple, int predicate_index) const;

    std::vector<int> unpack_tuple(long tuple, int predicate_index) const;

    int get_index_given_predicate_and_param(int pred, int param, int element) const;

    int get_obj_given_predicate_and_param(int pred, int param, int element) const;


    std::vector<std::vector<long>> hash_multipliers;
    std::vector<std::vector<std::unordered_map<int, int>>> obj_to_hash_index;
    std::vector<std::vector<std::unordered_map<int, int>>> hash_index_to_obj;
};


#endif //SEARCH_SPARSE_STATES_H
