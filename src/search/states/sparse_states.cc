
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
    if (packed_relations.size() != b.packed_relations.size())
        return false;
    for (size_t j = 0; j < packed_relations.size(); j++) {
        if (packed_relations[j] != b.packed_relations[j])
            return false;
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
    next_idx(0),
    num_predicates(task.predicates.size()) {}

SparsePackedState SparseStatePacker::pack(const DBState &state) {
    SparsePackedState packed_state(state.get_number_objects());
    const auto &relations = state.get_relations();
    packed_state.packed_relations.reserve(num_predicates);
    packed_state.nullary_atoms = state.get_nullary_atoms();
    for (const Relation &r : relations) {
        int predicate_index = r.predicate_symbol;
        std::vector<int> packed_relation;
        packed_relation.reserve(r.tuples.size());
        for (const auto &tuple : r.tuples) {
            packed_state.packed_relations.push_back(pack_tuple(tuple, predicate_index));
        }
        sort(packed_state.packed_relations.begin(), packed_state.packed_relations.end());
    }
    return packed_state;
}

DBState SparseStatePacker::unpack(const SparsePackedState &packed_state) const {
    std::vector<Relation> relations;
    std::vector<bool> nullary_atoms = packed_state.nullary_atoms;
    relations.reserve(num_predicates);
    std::vector<std::unordered_set<GroundAtom, TupleHash>> tuples(num_predicates, std::unordered_set<GroundAtom, TupleHash>());
    for (const auto &r : packed_state.packed_relations) {
        auto p = unpack_tuple(r);
        tuples[p.first].insert(p.second);
    }
    for (size_t i = 0; i < tuples.size(); ++i) {
        relations.emplace_back(i, std::move(tuples[i]));
    }
    return DBState(std::move(relations), std::move(nullary_atoms), packed_state.get_number_objects());
}

int SparseStatePacker::pack_tuple(const std::vector<int> &tuple, int predicate_index) {
    auto p = atom_index.try_emplace(make_pair(predicate_index, tuple), next_idx);
    int index = p.first->second;
    if (p.second) {
        index_to_atom.emplace(next_idx, make_pair(predicate_index, tuple));
        next_idx++;
    }
    return index;
}

std::pair<int, GroundAtom> SparseStatePacker::unpack_tuple(int index) const {
    return index_to_atom.at(index);
}


