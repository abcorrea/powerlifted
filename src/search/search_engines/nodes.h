#ifndef NODES_H
#define NODES_H

#include "../action.h"

#include "../heuristics/heuristic.h"

#include <cassert>
#include <iostream>
#include <utility>


class StateID {
    friend std::ostream &operator<<(std::ostream &os, StateID id);
    template <typename StateT>
    friend class SearchSpace;

    int value;
    explicit StateID(int value) : value(value) {}

public:
    StateID() = delete;
    ~StateID() = default;

    int id() const { return value; }

    static const StateID no_state;

    bool operator==(const StateID &other) const { return value == other.value; }
    bool operator!=(const StateID &other) const { return !(*this == other); }
    bool operator<(const StateID &other) const { return value < other.value; }
};


class SearchNode {
public:
    enum Status { NEW = 0, OPEN = 1, CLOSED = 2, DEAD_END = 3 };

    explicit SearchNode(StateID state_id) :
        SearchNode(state_id, LiftedOperatorId::no_operator, StateID::no_state, 0)
    {}

    SearchNode(StateID state_id, LiftedOperatorId op, StateID parent_state_id, int f)
        : state_id(state_id),
          op(std::move(op)),
          parent_state_id(parent_state_id),
          status(Status::NEW),
          f(f),
          g(0),
          h(0)
    {}

    void open(int f_) {
        status = SearchNode::Status::OPEN;
        f = f_;
    }

    void open(int g_, int h_) {
        status = SearchNode::Status::OPEN;
        g = g_;
        h = h_;
        f = g_ + h_;
    }

    void update_f(int f_) {
        f = f_;
    }

    void update_h(int h_) {
        h = h_;
        f = g + h;
    }

    void mark_as_unsolvable() {
        h = UNSOLVABLE_STATE;
        f = h;
    }

    void close() {
        assert(status == SearchNode::Status::OPEN);
        status = SearchNode::Status::CLOSED;
    }

    StateID state_id;
    LiftedOperatorId op;
    StateID parent_state_id;
    unsigned int status : 2;
    int f : 30;
    int g;
    int h;
};

#endif /*NODE_H*/