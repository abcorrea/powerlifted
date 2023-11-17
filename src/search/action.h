#ifndef SEARCH_ACTION_H
#define SEARCH_ACTION_H

#include <cassert>
#include <unordered_map>
#include <utility>
#include <vector>
#include <ostream>

/*
 * The Action class represents a grounded action.
 * The attribute index is the action schema index, from where we can retrieve its name and number of
 * parameters. The attribute instantiation is a list of object indices that instantiate each
 * argument of the corresponding action schema, in order as they appear in the action schema.
 */

class LiftedOperatorId {
    int index;
    std::vector<int> instantiation;
    std::unordered_map<int, int> fresh_vars;

public:
    static const LiftedOperatorId no_operator;

    LiftedOperatorId(int index, std::vector<int> &&instantiation, std::unordered_map<int, int> fresh_vars)
        : index(index), instantiation(std::move(instantiation)), fresh_vars(fresh_vars)
    {}

    LiftedOperatorId() = delete;


    int get_index() const {
        return index;
    }

    const std::vector<int>& get_instantiation() const {
        return instantiation;
    }

    int get_fresh_var_instantiation(int i) const {
        assert(fresh_vars.count(i) > 0);
        return fresh_vars.at(i);
    }

    const std::unordered_map<int, int> get_fresh_vars_mapping() const {
        return fresh_vars;
    }

    bool operator==(const LiftedOperatorId &other) const { return index == other.index; }
    bool operator!=(const LiftedOperatorId &other) const { return !(*this == other); }
};

std::ostream &operator<<(std::ostream &os, const LiftedOperatorId& id);

#endif  // SEARCH_ACTION_H
