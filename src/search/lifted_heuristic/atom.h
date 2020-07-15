#ifndef GROUNDER_ATOM_H
#define GROUNDER_ATOM_H

#include "arguments.h"
#include "term.h"

#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace  lifted_heuristic {

class Atom {
    Arguments arguments;
    int predicate_index;
    int index;
    static int next_index;

public:
    Atom(Arguments arguments, int predicate_index) :
        arguments(Arguments(std::move(arguments))),
        predicate_index(predicate_index),
        index(next_index++) {}

    Atom() = default;

    const Arguments &get_arguments() const {
        return arguments;
    }

    int get_predicate_index() const {
        return predicate_index;
    }

    int get_index() const {
        return index;
    }

    Term argument(size_t i) const {
        assert(i < arguments.size());
        return arguments[i];
    }

};

}

#endif //GROUNDER_ATOM_H
