#include <utility>

#include <utility>

#ifndef SEARCH_STRUCTURES_H
#define SEARCH_STRUCTURES_H

#include <string>
#include <vector>

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
    std::vector<Argument> tuples;
    bool negated;
};


#endif //SEARCH_STRUCTURES_H
