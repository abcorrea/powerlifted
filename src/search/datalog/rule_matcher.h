#ifndef GROUNDER__RULE_MATCHER_H_
#define GROUNDER__RULE_MATCHER_H_

#include "../parallel_hashmap/phmap.h"

#include <unordered_map>
#include <utility>
#include <vector>

namespace datalog {

class Match {
    int rule;
    int position;

public:
    Match(int r, int p) : rule(r), position(p) {}

    int get_rule() const {
        return rule;
    }
    int get_position() const {
        return position;
    }
};

class Matches {
    std::vector<Match> matches;

public:
    Matches() = default;

    explicit
    Matches(std::vector<Match> &&matches) : matches(std::move(matches)) {}

    void insert_new_match(int r, int p) {
        matches.emplace_back(r, p);
    }

    std::vector<Match>::const_iterator begin() const {
        return matches.begin();
    }

    std::vector<Match>::const_iterator end() const {
        return matches.end();
    }

};

class RuleMatcher {
    /*
     * Map predicate index to the rule matches. Predicate indices are small
     * dense integers, so a flat vector answers the per-pop matcher lookup,
     * the hottest lookup of the grounder loop, with a single indexed load
     * where a hash map would pay a hash and a probe per popped fact.
    */
    std::vector<Matches> rule_matcher;

    static const Matches empty_matches;

public:
    RuleMatcher() = default;

    void insert(int predicate_index, int rule_index, int position) {
        if (predicate_index >= int(rule_matcher.size())) {
            rule_matcher.resize(predicate_index + 1);
        }
        rule_matcher[predicate_index].insert_new_match(rule_index, position);
    }

    const Matches &get_matched_rules(int index) const {
        if (index >= int(rule_matcher.size()))
            return empty_matches;
        return rule_matcher[index];
    }

};

}

#endif //GROUNDER__RULE_MATCHER_H_