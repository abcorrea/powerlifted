#ifndef SEARCH_STRUCTURES_H
#define SEARCH_STRUCTURES_H

#include "hash_structures.h"

#include "utils/small_vector.h"

#include <string>
#include <utility>
#include <unordered_set>
#include <vector>

/**
 * @brief GroundAtom is a list of object indices (one per predicate argument).
 *
 * Ground atoms almost always have a small arity, so we store them inline with a
 * small-buffer-optimized vector. This avoids a heap allocation per atom on the
 * hot successor-generation path (every relation tuple and join-table tuple is a
 * GroundAtom), and keeps the data contiguous for hashing/compare. Same value
 * semantics as the previous std::vector<int>: same elements, order, ==, and
 * content-based hash (see TupleHash / utils::feed). The inline capacity is also
 * shared by Table::tuple_t so the two interconvert by move (see database/table.h).
 */
using GroundAtom = utils::small_vector<int, 4>;


/**
 * @brief Represent a parameter for a given action schema.
 *
 * @var name: Name of the parameter in the action schema definition.
 * @var index: Index of this parameter in the list of parameters in the schema.
 * @var type: Type related to this parameter.
 */
struct Parameter {
    Parameter(std::string name, int index, int type)
            : name(std::move(name)), index(index), type(type) {}

    std::string name;
    int index;
    int type;

    int get_index() const {
        return index;
    }

    bool operator<(const Parameter& other) const {
        return index < other.index;
    }

    bool operator==(const Parameter& other) const {
        return index == other.index;
    }

    bool operator!=(const Parameter& other) const {
        return index != other.index;
    }
};

namespace std {
template <>
struct hash<Parameter> {
    size_t operator()(const Parameter &parameter) const {
        return hash<int>()(parameter.get_index());
    }
};
}  // namespace std

/**
 * @brief Implements an argument composing an atom. It can be a free
 * variable or a constant. If it is a variable, it can either be a fresh variable or
 * a state variable.
 *
 * @var index: If the argument is a constant, then it represents the index of the object,
 * otherwise it represents the index of the free variable in the parameters of the action
 * schema.
 * @var constant: Indicates whether the argument is a constanst or not (free variable, then).
 *
 */
class Argument {
    int index;
    bool constant;
    bool is_fresh;
    std::string name;
public:
    Argument(int index, bool constant, bool is_fresh) :
        index(index),
        constant(constant),
        is_fresh(is_fresh) {}

    int get_index() const {
        return index;
    }

    bool is_constant() const {
        return constant;
    }

    bool is_fresh_var() const {
        return is_fresh;
    }

};

class FreshVariable : public Argument {
    std::string name;
public:
    FreshVariable(std::string name, int index, bool constant) :
        Argument(index, constant, true),
        name(name){}

    const std::string &get_name() const {
        return name;
    }

    bool is_fresh_var() const {
        return true;
    }

};

/**
 * @brief A relation is a "table" with set of tuples corresponding to some
 * predicate in a state.
 *
 * @var predicate_symbol: Indicates its corresponding predicate.
 * @var tuples: Set of tuples (vectors) corresponding to the ground atoms in this relation.
 *
 */
struct Relation {
    Relation() = default;
    Relation(int predicate_symbol,
        std::unordered_set<GroundAtom, TupleHash> &&tuples)
            : predicate_symbol(predicate_symbol),
              tuples (std::move(tuples)) {}

    Relation(const Relation &) = default;


    bool operator==(const Relation &other) const {
        return predicate_symbol == other.predicate_symbol && tuples == other.tuples;
    }

    int predicate_symbol{};
    std::unordered_set<GroundAtom, TupleHash> tuples;
};

#endif //SEARCH_STRUCTURES_H
