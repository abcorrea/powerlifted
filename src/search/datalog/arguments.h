#ifndef GROUNDER_ARGUMENTS_H
#define GROUNDER_ARGUMENTS_H

#include "term.h"

#include "../utils/hash.h"

#include <boost/container/small_vector.hpp>

#include <cassert>
#include <vector>

namespace datalog {

class Arguments {
    // Datalog facts almost always have a handful of arguments (predicate arity),
    // so we keep them inline with a small-buffer-optimized vector. This avoids a
    // tiny heap allocation per Fact and keeps the argument data contiguous with
    // the Fact itself — every fact hash/compare/copy in the grounder fixpoint
    // touches these arguments, so cutting the indirection matters more than the
    // raw malloc count. Behaviour is identical to a std::vector<Term>: same
    // elements, same order, same operator== and hash_range result.
    using Container = boost::container::small_vector<Term, 4>;
    Container arguments;

public:
    Arguments() = default;

    explicit Arguments(const std::vector<std::pair<int, int>> &args)
    {
        for (const auto &p : args) {
            arguments.emplace_back(p.first, p.second);
        }
    }

    explicit Arguments(std::vector<Term> &&args)
        : arguments(args.begin(), args.end()) {}

    Term operator[](size_t i) const
    {
        assert(i < arguments.size());
        return arguments[i];
    }

    Container::const_iterator begin() const { return arguments.begin(); }

    Container::const_iterator end() const { return arguments.end(); }

    size_t size() const { return arguments.size(); }

    void push_back(int i, int j) { arguments.emplace_back(i, j); };

    void set_term_to_object(int i, int j) { arguments[i].set_term_to_object(j); }

    bool is_object(std::size_t i) const { return arguments[i].is_object(); }

    bool operator==(const Arguments &b) const { return arguments == b.arguments; }

    bool operator!=(const Arguments &b) const { return not(arguments == b.arguments); }
};

class HashArguments {
public:
    std::size_t operator()(const Arguments &f) const
    {
        return utils::hash_range(f.begin(), f.end());
    }
};

}  // namespace datalog

#endif  // GROUNDER_ARGUMENTS_H
