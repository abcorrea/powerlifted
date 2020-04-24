#ifndef SEARCH_ACTION_H
#define SEARCH_ACTION_H

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
  public:
    int index;
    std::vector<int> instantiation;

    static const LiftedOperatorId no_operator;

    LiftedOperatorId(int index, std::vector<int> &&instantiation)
        : index(index), instantiation(std::move(instantiation))
    {}

    LiftedOperatorId() = delete;

    bool operator==(const LiftedOperatorId &other) const { return index == other.index; }
    bool operator!=(const LiftedOperatorId &other) const { return !(*this == other); }
};

std::ostream &operator<<(std::ostream &os, const LiftedOperatorId& id);

#endif  // SEARCH_ACTION_H
