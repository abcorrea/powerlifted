
#include "sparse_states.h"
#include "../task.h"
#include "../utils.h"

#include "../utils/hash.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>



bool SparsePackedState::operator==(const SparsePackedState &b) const {
    for (size_t i = 0; i < nullary_atoms.size(); ++i) {
        if (nullary_atoms[i] != b.nullary_atoms[i])
            return false;
    }
    for (size_t i = 0; i < packed_relations.size(); ++i) {
        if (packed_relations[i].size() != b.packed_relations[i].size())
            return false;
        for (size_t j = 0; j < packed_relations[i].size(); j++) {
            if (packed_relations[i][j] != b.packed_relations[i][j])
                return false;
        }
    }
    return true;
}


unsigned PackedStateHash::operator() (const SparsePackedState &s) const {
    utils::HashState hash_state;

    for (bool b : s.nullary_atoms) {
        utils::feed(hash_state, b);
    }
    for (const auto &r : s.packed_relations) {
        utils::feed(hash_state, r);
    }
    return hash_state.get_hash32();
}


SparseStatePacker::SparseStatePacker(const Task &task) :
    atom_index(task.predicates.size()),
    index_to_atom(task.predicates.size()),
    next_idx(0) {
}

SparsePackedState SparseStatePacker::pack(const DBState &state) {
    SparsePackedState packed_state;
    const auto &relations = state.get_relations();
    packed_state.packed_relations.reserve(relations.size());
    packed_state.nullary_atoms = state.get_nullary_atoms();
    for (const Relation &r : relations) {
        int predicate_index = r.predicate_symbol;
        std::vector<int> packed_relation;
        packed_relation.reserve(r.tuples.size());
        for (const auto &tuple : r.tuples) {
            packed_relation.push_back(pack_tuple(tuple, predicate_index));
        }
        sort(packed_relation.begin(), packed_relation.end());
        packed_state.packed_relations.push_back(packed_relation);
    }
    return packed_state;
}

DBState SparseStatePacker::unpack(const SparsePackedState &packed_state) const {
    std::vector<Relation> relations;
    std::vector<bool> nullary_atoms = packed_state.nullary_atoms;
    relations.reserve(packed_state.packed_relations.size());
    for (size_t i = 0; i < packed_state.packed_relations.size(); ++i) {
        std::unordered_set<GroundAtom, TupleHash> tuples;
        for (const auto &r : packed_state.packed_relations[i]) {
            tuples.insert(unpack_tuple(r, i));
        }
        relations.emplace_back(i, std::move(tuples));
    }
    return DBState(std::move(relations), std::move(nullary_atoms));
}

long SparseStatePacker::pack_tuple(const std::vector<int> &tuple, int predicate_index) {
    auto p = atom_index[predicate_index].try_emplace(tuple, next_idx);
    long index = p.first->second;
    if (p.second) {
        index_to_atom[predicate_index].emplace(next_idx, tuple);
        next_idx++;
    }
    return index;
}

GroundAtom SparseStatePacker::unpack_tuple(long tuple, int predicate_index) const {
    return index_to_atom[predicate_index].at(tuple);
}


