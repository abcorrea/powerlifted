//
// Created by blaas on 01.11.21.
//

#ifndef SEARCH_DATALOG_ACHIEVERS_H_
#define SEARCH_DATALOG_ACHIEVERS_H_

namespace datalog {

class Achievers {
    std::vector<int> achievers;
    int rule_idx;
    int rule_cost;

public:
    Achievers(const std::vector<int> &&a, int idx, int c) : achievers(std::move(a)), rule_idx(idx), rule_cost(c) {};

    Achievers() : achievers(), rule_idx(-1), rule_cost(0) {}

    std::vector<int>::const_iterator begin() const {
        return achievers.begin();
    }

    std::vector<int>::const_iterator end() const {
        return achievers.end();
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
