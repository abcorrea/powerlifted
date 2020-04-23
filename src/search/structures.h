#ifndef SEARCH_STRUCTURES_H
#define SEARCH_STRUCTURES_H

#include "hash_structures.h"

#include <string>
#include <utility>
#include <unordered_set>
#include <vector>

#include <boost/functional/hash.hpp>

/**
 * @brief GroundAtom is an alias for vector of integers. It is represented
 * as a list of object indices.
 */
typedef std::vector<int> GroundAtom;


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
};


/**
 * @brief Implements an argument composing an atom. It can be a free
 * variable or a constant.
 *
 * @var index: If the argument is a constant, then it represents the index of the object,
 * otherwise it represents the index of the free variable in the parameters of the action
 * schema.
 * @var constant: Indicates whether the argument is a constanst or not (free variable, then).
 *
 */
struct Argument {
    Argument(int index, bool constant) : index(index), constant(constant) {}
    int index;
    bool constant;
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


/**
 * @brief Represent a lifted atom by its name, predicate symbol index,
 * list of arguments, and whether it is negated or not.
 *
 * @var name: String representing atom name
 * @var predicate_symbol: predicate symbol index
 * @var arguments: list of Argument objects representing the free variables
 * or constants of the atom
 * @var negated: boolean variable indicating whether the atom is negated
 * or not (in whatever context it occurs)
 *
 * @see Argument (structures.h)
 */
struct Atom {
    Atom(std::string &&name, int predicate_symbol,
        std::vector<Argument> &&tuples, bool negated) :
            name(std::move(name)),
            predicate_symbol(predicate_symbol),
            arguments(std::move(tuples)),
            negated(negated) {}

    std::string name;
    int predicate_symbol;
    std::vector<Argument> arguments;
    bool negated;
};


#endif //SEARCH_STRUCTURES_H
