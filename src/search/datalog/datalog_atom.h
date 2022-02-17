#ifndef GROUNDER_ATOM_H
#define GROUNDER_ATOM_H

#include "arguments.h"
#include "object.h"
#include "term.h"

#include "../atom.h"
#include "../action_schema.h"

#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace  datalog {

class DatalogAtom {
    Arguments arguments;
    int predicate_index;
    int index;
    bool new_pred_symbol; // If atom has a predicate symbol that is not an atom in the task
    static int next_index;

public:
    DatalogAtom(Arguments arguments, int predicate_index, bool new_pred_symbol) :
        arguments(Arguments(std::move(arguments))),
        predicate_index(predicate_index),
        index(next_index++),
        new_pred_symbol(new_pred_symbol) {}

    DatalogAtom(const Atom &atom);

    DatalogAtom(const ActionSchema &schema, int idx);

    DatalogAtom() = default;

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

    bool is_pred_symbol_new() const {
        return new_pred_symbol;
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

    bool is_nullary() const;

    bool is_ground() const;

    friend bool operator==(const DatalogAtom &lhs, const DatalogAtom &rhs) {
        if (lhs.predicate_index != rhs.predicate_index) return false;
        //if (lhs.index != rhs.predicate_index) return false;
        return !(lhs.arguments!=rhs.arguments);
    }

    void update_arguments(std::vector<Term> &terms) {
        arguments = Arguments(std::move(terms));
    }

    bool share_variables(const DatalogAtom &atom) const;
};

}

#endif //GROUNDER_ATOM_H
