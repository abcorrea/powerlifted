#ifndef GROUNDER_ARGUMENTS_H
#define GROUNDER_ARGUMENTS_H

#include "term.h"

#include <vector>

namespace datalog {

class Arguments {
    std::vector<Term> arguments;

public:
    Arguments() = default;

    explicit
    Arguments(const std::vector<std::pair<int, int>> &args) {
        for (const auto &p : args) {
            arguments.emplace_back(p.first, p.second);
        }
    }

    explicit Arguments(std::vector<Term> &&args) : arguments(args) {}

    Term operator[](size_t i) const {
        assert(i < arguments.size());
        return arguments[i];
    }

    std::vector<Term>::const_iterator begin() const {
        return arguments.begin();
    }

    std::vector<Term>::const_iterator end() const {
        return arguments.end();
    }

    size_t size() const {
        return arguments.size();
    }

    void push_back(int i, int j) {
        arguments.emplace_back(i, j);
    };

    void set_term_to_object(int i, int j) {
        arguments[i].set_term_to_object(j);
    }

    bool is_object(std::size_t i) const {
        return arguments[i].is_object();
    }

    bool operator==(const Arguments &b) const {
        return arguments == b.arguments;
    }

    bool operator!=(const Arguments &b) const {
        return not (arguments == b.arguments);
    }

};

class HashArguments {
public:
    std::size_t operator()(const Arguments &f) const {
        return boost::hash_range(f.begin(), f.end());
    }
};

}

#endif //GROUNDER_ARGUMENTS_H
