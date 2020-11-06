#ifndef GROUNDER_ATOM_H
#define GROUNDER_ATOM_H

#include "arguments.h"
#include "object.h"
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

    void print_atom(const std::vector<Object> &obj,
                          const std::unordered_map<int, std::string> &map_index_to_atom) const {
        std::cout << map_index_to_atom.at(predicate_index) << '(';
        size_t cont = 0;
        for (const Term &t : arguments) {
            if (t.is_object()) {
                // a >= 0 --> object. Print its name
                std::cout << obj[t.get_index()].get_name();
            } else {
                // a < 0 --> free variable. Print the free variable of the rule.
                std::cout << '?' << char('A' + t.get_index());
            }
            cont++;
            if (cont!=arguments.size())
                std::cout << ", ";
        }
        std::cout << ')'; //<< std::endl;
    }


};

}

#endif //GROUNDER_ATOM_H
