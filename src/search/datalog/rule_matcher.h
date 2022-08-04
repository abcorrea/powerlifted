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
     Map index of an atom to a vector rule matches
    */
    phmap::flat_hash_map<int, Matches> rule_matcher;

    static const Matches empty_matches;

    bool atom_has_matched_rules(int i) const {
        return (rule_matcher.find(i)!=rule_matcher.end());
    }

public:
    RuleMatcher() = default;

//    explicit RuleMatcher(std::unordered_map<int, Matches> rule_matcher) :
//        rule_matcher(std::move(rule_matcher)) {}

    void insert(int predicate_index, int rule_index, int position) {
        if (!atom_has_matched_rules(predicate_index)) {
            rule_matcher[predicate_index] = Matches();
        }
        rule_matcher[predicate_index].insert_new_match(rule_index, position);
    }

    const Matches &get_matched_rules(int index) const {
        auto result = rule_matcher.find(index);
        if (result==rule_matcher.end())
            return empty_matches;
        return result->second;
    }

};

}

#endif //GROUNDER__RULE_MATCHER_H_