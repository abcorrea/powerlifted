#include <utility>

#ifndef SEARCH_STRUCTURES_H
#define SEARCH_STRUCTURES_H

#include <string>
#include <utility>
#include <vector>

#include <boost/functional/hash.hpp>

typedef std::vector<int> GroundAtom; // Ground atom is a list of object indices.

struct Parameter {
    Parameter(std::string name, int index, int type) : name(std::move(name)), index(index), type(type) {}

    std::string name;
    int index;
    int type;
};

struct Argument {
    Argument(int index, bool constant) : index(index), constant(constant) {}

    /*
     * An argument has a field for its parameter index.
     * It also has a flag indicating whether it is a constant or not.
     */
    int index;
    bool constant;
};

struct Relation {
    /*
     * A relation is a "table" corresponding to some predicate in a state.  The predicate_symbol attribute indicates
     * its corresponding predicate and the tuples attribute is a list of tuples, represented as a vector of vectors.
     */
    Relation(int predicate_symbol, std::vector<GroundAtom> tuples) : predicate_symbol(predicate_symbol),
                                                                     tuples (std::move(tuples)) {}

    Relation(const Relation &rhs) = default;

    Relation() = default;

    bool operator==(const Relation &other) const {
        bool vector_equal = true;
        if (tuples.size() != other.tuples.size()) {
            return false;
        }
        for (int i = 0; i < tuples.size(); ++i) {
            if (tuples[i] != other.tuples[i])
                vector_equal = false;
        }
        return predicate_symbol == other.predicate_symbol
                && vector_equal;
    }

    int predicate_symbol;
    std::vector<GroundAtom> tuples;
};

struct Atom {
    Atom(std::string name, int predicate_symbol, std::vector<Argument> tuples, bool negated) :
            name(std::move(name)),
            predicate_symbol(predicate_symbol),
            tuples(std::move(tuples)),
            negated(negated) {}

    /*
     * Atom has a predicate, a list of arguments, a flag whether it is negated.
     */
    std::string name;
    int predicate_symbol;
    std::vector<Argument> tuples; // TODO change name to "arguments"
    bool negated;
};

/*
 * Hash functions
 */
/*
std::size_t hash_value(const Relation &r) {
    boost::hash<std::vector<std::vector<int>>> h1;
    return h1(r.tuples);
}*/

#endif //SEARCH_STRUCTURES_H
