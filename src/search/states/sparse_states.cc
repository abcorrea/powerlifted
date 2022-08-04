
#include "sparse_states.h"
#include "../task.h"
#include "../utils.h"

#include "../utils/hash.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>



bool SparsePackedState::operator==(const SparsePackedState &b) const {
    if (predicate_symbols.size() != b.predicate_symbols.size())
        return false;
    for (size_t i = 0; i < predicate_symbols.size(); ++i) {
        if (predicate_symbols[i] != b.predicate_symbols[i])
            return false;
    }
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

    for (int i : s.predicate_symbols) {
        utils::feed(hash_state, i);
    }
    for (bool b : s.nullary_atoms) {
        utils::feed(hash_state, b);
    }
    for (const auto &r : s.packed_relations) {
        utils::feed(hash_state, r);
    }
    return hash_state.get_hash32();
}


SparseStatePacker::SparseStatePacker(const Task &task) {
    obj_to_hash_index.resize(task.predicates.size());
    hash_index_to_obj.resize(task.predicates.size());
    hash_multipliers.resize(task.predicates.size());
    auto objects_per_type = task.compute_object_index();


    // Loop over all predicates computing the hash multipliers
    // for each one.
    for (size_t i = 0; i < hash_multipliers.size(); ++i) {
        const Predicate &pred = task.predicates[i];
        hash_multipliers[i].reserve(pred.getTypes().size());
        obj_to_hash_index[i].resize(pred.getTypes().size());
        hash_index_to_obj[i].resize(pred.getTypes().size());
        long multiplier = 1;
        int cont = 0;
        for (auto t : pred.getTypes()) {
            hash_multipliers[i].push_back(multiplier);
            if (is_product_within_limit(multiplier, objects_per_type[t].size(),
                                        std::numeric_limits<long>::max())) {
                multiplier *= objects_per_type[t].size();
                for (size_t j = 0; j < objects_per_type[t].size(); ++j) {
                    obj_to_hash_index[i][cont][objects_per_type[t][j]] = j;
                    hash_index_to_obj[i][cont][j] = objects_per_type[t][j];
                }
                cont++;
            }
            else {
                std::cerr << "Hash multipliers overflow!" <<
                " State representation is too large to be packed!" << std::endl;
                exit(-2);
            }
        }
    }
}

SparsePackedState SparseStatePacker::pack(const DBState &state) const {
    SparsePackedState packed_state;
    const auto &relations = state.get_relations();
    packed_state.packed_relations.reserve(relations.size());
    packed_state.predicate_symbols.reserve(relations.size());
    packed_state.nullary_atoms = state.get_nullary_atoms();
    for (const Relation &r : relations) {
        std::vector<long> packed_relation;
        packed_relation.reserve(r.tuples.size());
        int predicate_index = r.predicate_symbol;
        packed_state.predicate_symbols.push_back(predicate_index);
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
            tuples.insert(unpack_tuple(r, packed_state.predicate_symbols[i]));
        }
        relations.emplace_back(packed_state.predicate_symbols[i], std::move(tuples));
    }
    return DBState(std::move(relations), std::move(nullary_atoms));
}

long SparseStatePacker::pack_tuple(const std::vector<int> &tuple, int predicate_index) const {
    long index = 0;
    for (size_t i = 0; i < tuple.size(); ++i) {
        index += hash_multipliers[predicate_index][i] *
                 get_index_given_predicate_and_param(predicate_index, i, tuple[i]);
    }
    return index;
}

GroundAtom SparseStatePacker::unpack_tuple(long tuple, int predicate_index) const {
    std::vector<int> values(hash_multipliers[predicate_index].size());
    int aux;
    for (int i = hash_multipliers[predicate_index].size() - 1; i >= 0; --i) {
        aux = tuple / hash_multipliers[predicate_index][i];
        values[i] = get_obj_given_predicate_and_param(predicate_index, i, aux);
        tuple -= aux * hash_multipliers[predicate_index][i];
    }
    assert(tuple == 0);
    return values;
}

int SparseStatePacker::get_index_given_predicate_and_param(int pred, int param, int element) const {
    return obj_to_hash_index[pred][param].at(element);
}

int SparseStatePacker::get_obj_given_predicate_and_param(int pred, int param, int element) const {
    return hash_index_to_obj[pred][param].at(element);
}


