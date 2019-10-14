#ifndef SEARCH_STATE_PACKER_H
#define SEARCH_STATE_PACKER_H

#include <iostream>
#include <vector>

#include "task.h"
#include "utils.h"


struct PackedState {
    std::vector<std::vector<int>> packed_relations;
    std::vector<int> predicate_symbols;
    std::vector<bool> nullary_atoms;


    bool operator==(const PackedState &b) const{
        if (predicate_symbols.size() != b.predicate_symbols.size())
            return false;
        for (int i = 0; i < predicate_symbols.size(); ++i) {
            if (predicate_symbols[i] != b.predicate_symbols[i])
                return false;
        }
        for (int i = 0; i < nullary_atoms.size(); ++i) {
            if (nullary_atoms[i] != b.nullary_atoms[i])
                return false;
        }
        for (int i = 0; i < packed_relations.size(); ++i) {
            if (packed_relations[i].size() != b.packed_relations[i].size())
                return false;
            for (int j = 0; j < packed_relations[i].size(); j++) {
                if (packed_relations[i][j] != b.packed_relations[i][j])
                    return false;
            }
        }
        return true;
    }

};

struct PackedStateHash {
    std::size_t operator() (const PackedState &s) const {
        std::size_t seed = 0;
        for (int i : s.predicate_symbols) {
            boost::hash_combine(seed, i);
        }
        for (bool b : s.nullary_atoms) {
            boost::hash_combine(seed, b);
        }
        for (const auto &r : s.packed_relations) {
            size_t x = boost::hash_range(r.begin(), r.end());
            boost::hash_combine(seed, x);
        }
        return seed;
    }
};


/**
 * @brief Pack and unpack states into a more compact representation
 */

class StatePacker {
public:
    StatePacker(const Task &task) {

        // First, create a vector with one bucket for each type and
        // count the number of objects in each bucket.
        std::vector<int> objects_per_type(task.type_names.size(), 0);
        hash_multipliers.resize(task.predicates.size());
        for (const Object &o : task.objects) {
            int i = o.getIndex();
            for (int t : o.getTypes()) {
                objects_per_type[t]++;
            }
        }

        // Loop over all predicates computing the hash multipliers
        // for each one.
        for (int i = 0; i < hash_multipliers.size(); ++i) {
            const Predicate &pred = task.predicates[i];
            hash_multipliers[i].reserve(pred.getTypes().size());
            int multiplier = 1;
            for (auto t : pred.getTypes()) {
                hash_multipliers[i].push_back(multiplier);
                if (is_product_within_limit(multiplier, objects_per_type[i],
                                            numeric_limits<int>::max())) {
                    multiplier *= objects_per_type[t];
                }
                else {
                    std::cerr << "Hash multipliers overflow! State representation if too large to be packed!" << endl;
                    exit(-2);
                }
            }
        }
    }

    PackedState pack_state(const State &state) const {
        PackedState packed_state;
        packed_state.packed_relations.reserve(state.relations.size());
        packed_state.predicate_symbols.reserve(state.relations.size());
        packed_state.nullary_atoms = state.nullary_atoms;
        for (const Relation &r : state.relations) {
            std::vector<int> packed_relation;
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

    State unpack_state(const PackedState &packed_state) const {
        std::vector<Relation> relations;
        std::vector<bool> nullary_atoms = packed_state.nullary_atoms;
        relations.reserve(packed_state.packed_relations.size());
        for (int i = 0; i < packed_state.packed_relations.size(); ++i) {
            std::unordered_set<GroundAtom, TupleHash> tuples;
            for (const auto &r : packed_state.packed_relations[i]) {
                tuples.insert(unpack_tuple(r, packed_state.predicate_symbols[i]));
            }
            relations.emplace_back(packed_state.predicate_symbols[i], move(tuples));
        }
        return State(move(relations), move(nullary_atoms));
    }

private:
    int pack_tuple(const std::vector<int> &tuple, int predicate_index) const {
        size_t index = 0;
        for (size_t i = 0; i < tuple.size(); ++i) {
            index += hash_multipliers[predicate_index][i] * tuple[i];
        }
        return index;
    }

    GroundAtom unpack_tuple(int tuple, int predicate_index) const {
        std::vector<int> values(hash_multipliers[predicate_index].size());
        for (int i = hash_multipliers[predicate_index].size() - 1; i >= 0; --i) {
            values[i] = tuple / hash_multipliers[predicate_index][i];
            tuple -= values[i] * hash_multipliers[predicate_index][i];
        }
        assert(tuple == 0);
        return values;
    }




    std::vector<std::vector<int>> hash_multipliers;

};


#endif //SEARCH_STATE_PACKER_H
