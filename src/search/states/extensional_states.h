#ifndef EXTENSIONAL_SEARCH_STATE_PACKER_H
#define EXTENSIONAL_SEARCH_STATE_PACKER_H

#include "../task.h"
#include "../utils.h"

#include "../utils/hash.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>


/**
 * @brief A bitvector-based representation of states
 *
 * @details
 *
 */

struct ExtensionalPackedState {
    std::vector<bool> atoms;

    bool operator==(const ExtensionalPackedState &b) const { return atoms == b.atoms; }

    struct Hash {
        std::size_t operator() (const ExtensionalPackedState &s) const {
            return boost::hash_range(s.atoms.cbegin(), s.atoms.cend());
        }
    };
};


/**
 * @brief Pack and unpack states into a more compact representation
 */
/*
class StatePacker {
public:
    explicit StatePacker(const Task &task) {

        // First, create a vector with one bucket for each type and
        // count the number of objects in each bucket.
        std::vector<std::vector<int>> objects_per_type(task.type_names.size());
        obj_to_hash_index.resize(task.predicates.size());
        hash_index_to_obj.resize(task.predicates.size());
        hash_multipliers.resize(task.predicates.size());
        for (const Object &o : task.objects) {
            for (int t : o.getTypes()) {
                objects_per_type[t].push_back(o.getIndex());
            }
        }


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
                                            numeric_limits<long>::max())) {
                    multiplier *= objects_per_type[t].size();
                    for (size_t j = 0; j < objects_per_type[t].size(); ++j) {
                        obj_to_hash_index[i][cont][objects_per_type[t][j]] = j;
                        hash_index_to_obj[i][cont][j] = objects_per_type[t][j];
                    }
                    cont++;
                }
                else {
                    std::cerr << "Hash multipliers overflow!" <<
                    " State representation is too large to be packed!" << endl;
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

    State unpack_state(const PackedState &packed_state) const {
        std::vector<Relation> relations;
        std::vector<bool> nullary_atoms = packed_state.nullary_atoms;
        relations.reserve(packed_state.packed_relations.size());
        for (size_t i = 0; i < packed_state.packed_relations.size(); ++i) {
            std::unordered_set<GroundAtom, TupleHash> tuples;
            for (const auto &r : packed_state.packed_relations[i]) {
                tuples.insert(unpack_tuple(r, packed_state.predicate_symbols[i]));
            }
            relations.emplace_back(packed_state.predicate_symbols[i], move(tuples));
        }
        return State(move(relations), move(nullary_atoms));
    }

protected:
    long pack_tuple(const std::vector<int> &tuple, int predicate_index) const {
        long index = 0;
        for (size_t i = 0; i < tuple.size(); ++i) {
            index += hash_multipliers[predicate_index][i] * get_index_given_predicate_and_param(predicate_index,
                                                                                              i, tuple[i]);
        }
        return index;
    }

    GroundAtom unpack_tuple(long tuple, int predicate_index) const {
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

    int get_index_given_predicate_and_param(int pred, int param, int element) const {
        return obj_to_hash_index[pred][param].at(element);
    }

    int get_obj_given_predicate_and_param(int pred, int param, int element) const {
        return hash_index_to_obj[pred][param].at(element);
    }


    std::vector<std::vector<long>> hash_multipliers;
    std::vector<std::vector<std::unordered_map<int, int>>> obj_to_hash_index;
    std::vector<std::vector<std::unordered_map<int, int>>> hash_index_to_obj;

};

*/
#endif // EXTENSIONAL_SEARCH_STATE_PACKER_H
