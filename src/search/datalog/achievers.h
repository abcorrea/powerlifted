//
// Created by blaas on 01.11.21.
//

#ifndef SEARCH_DATALOG_ACHIEVERS_H_
#define SEARCH_DATALOG_ACHIEVERS_H_

#include "../utils/small_vector.h"

namespace datalog {

class Achievers {
    // A fact's achiever body is tiny (one entry for project rules, two for join
    // rules), so we hold it inline to avoid a per-fact heap allocation. Same
    // semantics as the previous std::vector<int>.
    using Container = utils::small_vector<int, 2>;
    Container achievers;
    int rule_idx;
    int rule_cost;

public:

    // Copy constructor used only for the product rules
    Achievers(const std::vector<int> &a, int rule_idx, int rule_cost) : achievers(a.begin(), a.end()), rule_idx(rule_idx), rule_cost(rule_cost) {};

    Achievers(const std::vector<int> &&a, int rule_idx, int rule_cost) : achievers(a.begin(), a.end()), rule_idx(rule_idx), rule_cost(rule_cost) {};

    Achievers() : achievers(), rule_idx(-1), rule_cost(0) {}

    Container::const_iterator begin() const {
        return achievers.begin();
    }

    Container::const_iterator end() const {
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
