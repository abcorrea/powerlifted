#ifndef SEARCH_STATE_H
#define SEARCH_STATE_H

#include "../structures.h"

#include <algorithm>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

/**
 * @brief Represents a state in the search space. Intuitively, it is represented
 * as a list of relations (tables).
 *
 * @details A state has a vector of Relations and a boolean vector indicating
 * which nullary atoms are true in the given state. This representation is sometimes
 * called 'sparse', because we do not have a value for every (relaxed) reachable
 * ground atom of the task.
 * For motivation on the use of sparse state representation,
 * see A. B. Correa, 2019.'Planning using Lifted Task Representations',
 * M.Sc. thesis. University of Basel.
 *
 * @see state_packer.h
 *
 */
class DBState {

    std::vector<Relation> relations;
    std::vector<bool> nullary_atoms;

public:

    DBState() = default;
    explicit DBState(unsigned num_predicates) :
        relations(num_predicates), nullary_atoms(num_predicates, false) {}

    DBState(std::vector<Relation> &&relations, std::vector<bool> &&nullary_atoms) :
        relations(std::move(relations)), nullary_atoms(std::move(nullary_atoms)) {
        // Explicit state constructor
    }

    const std::vector<Relation>& get_relations() const {
        return relations;
    }

    const std::vector<bool> &get_nullary_atoms() const {
        return nullary_atoms;
    }

    const std::unordered_set<GroundAtom, TupleHash>& get_tuples_of_relation(size_t i) const {
        return relations[i].tuples;
    }

    void set_nullary_atom(size_t index, bool v) {
        nullary_atoms[index] = v;
    }

    void set_relation_predicate_symbol(size_t i, int id) {
        relations[i].predicate_symbol = id;
    }

    void insert_tuple_in_relation(GroundAtom ga, int id) {
        relations[id].tuples.insert(ga);
    }

    void add_tuple(int relation, const GroundAtom &args);

    bool operator==(const DBState &other) const {
        return nullary_atoms==other.nullary_atoms && relations==other.relations;
    }

    friend std::size_t hash_value(const DBState &s);
};

/**
 * @brief Syntatic sugar to avoid passing the static predicated to
 * every single successor.
 */
typedef DBState StaticInformation;

#endif //SEARCH_STATE_H
