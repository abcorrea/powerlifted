//
// Created by blaas on 01.11.21.
//

#ifndef SEARCH_DATALOG_ACHIEVERS_H_
#define SEARCH_DATALOG_ACHIEVERS_H_

#include "../utils/small_vector.h"

#include <vector>

namespace datalog {

class Achievers {
    // The body of a project rule has one atom and a join rule has two, so an
    // inline capacity of 2 keeps their achiever bodies off the heap (this is
    // built once per newly reached fact, in the grounder hot loop). Product
    // rules, whose bodies can be longer, spill transparently.
    utils::small_vector<int, 2> achievers;
    int rule_idx;
    int rule_cost;

public:

    // Hot-path constructors: build the achiever body inline for one-atom
    // (project) and two-atom (join) rule bodies.
    Achievers(int a, int rule_idx, int rule_cost) : rule_idx(rule_idx), rule_cost(rule_cost) {
        achievers.push_back(a);
    }

    Achievers(int a, int b, int rule_idx, int rule_cost) : rule_idx(rule_idx), rule_cost(rule_cost) {
        achievers.push_back(a);
        achievers.push_back(b);
    }

    // Used by the product rules, whose body may have more than two atoms.
    Achievers(const std::vector<int> &a, int rule_idx, int rule_cost)
        : achievers(a.begin(), a.end()), rule_idx(rule_idx), rule_cost(rule_cost) {};

    Achievers() : achievers(), rule_idx(-1), rule_cost(0) {}

    utils::small_vector<int, 2>::const_iterator begin() const {
        return achievers.begin();
    }

    utils::small_vector<int, 2>::const_iterator end() const {
        return achievers.end();
    }

    int at(int i) const {
        return achievers[i];
    }

    void set_rule_cost(int c) {
        rule_cost = c;
    }

    void set_rule_index(int i) {
        rule_idx = i;
    }

    void push_back(int i) {
        achievers.push_back(i);
    }

    int get_achiever_rule_index() const {
        return rule_idx;
    }

    int get_achiever_rule_cost() const {
        return rule_cost;
    }

};
}

#endif //SEARCH_DATALOG_ACHIEVERS_H_
