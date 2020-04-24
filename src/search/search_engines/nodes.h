
#pragma once

#include "../action.h"
#include <iostream>


class StateID {
    friend std::ostream &operator<<(std::ostream &os, StateID id);
    template <typename StateT>
    friend class SearchSpace;

    int value;
    explicit StateID(int value) : value(value) {}

public:
    StateID() = delete;
    ~StateID() = default;

    static const StateID no_state;

    bool operator==(const StateID &other) const { return value == other.value; }
    bool operator!=(const StateID &other) const { return !(*this == other); }
};


class SearchNode {
public:
    enum Status { NEW = 0, OPEN = 1, CLOSED = 2, DEAD_END = 3 };

    StateID state_id;
    StateID parent_state_id;
    LiftedOperatorId creating_operator;
    unsigned int status : 2;
    int real_g : 30;
};

