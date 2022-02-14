#ifndef SEARCH_DATALOG_TRANSFORMATIONS_GREEDY_JOIN_H_
#define SEARCH_DATALOG_TRANSFORMATIONS_GREEDY_JOIN_H_

#include "../datalog.h"

#include "../rules/rule_base.h"
#include "../rules/generic_rule.h"
#include "../rules/product.h"
#include "../rules/project.h"

namespace datalog {

// TODO This is incomplete.

class JoinCost {
    int new_arity;
    int max_arity;
    int min_arity;

public:
    JoinCost(int n, int max, int min) : new_arity(n), max_arity(max), min_arity(min) {}

    JoinCost() : new_arity(std::numeric_limits<int>::max()),
                 max_arity(std::numeric_limits<int>::max()),
                 min_arity(std::numeric_limits<int>::max()) {}


    friend bool operator<(const JoinCost &lhs, const JoinCost &rhs) {
        if (lhs.new_arity != rhs.new_arity) {
            return (lhs.new_arity < rhs.new_arity);
        }
        if (lhs.max_arity != rhs.max_arity) {
            return (lhs.max_arity < rhs.max_arity);
        }
        return (lhs.min_arity < rhs.min_arity);
    }

};

Arguments compute_joining_variables(const std::unique_ptr<RuleBase> &rule, const DatalogAtom &atom1, const DatalogAtom &atom2) {
    const Arguments &args1 = atom1.get_arguments();
    const Arguments &args2 = atom2.get_arguments();
    std::unordered_set<Term, boost::hash<Term>> variables_from_removed_atoms;
    for (const auto &t : args1) {
        if (not t.is_object()) {
            variables_from_removed_atoms.insert(t);
        }
    }
    for (const auto &t : args2) {
        if (not t.is_object()) {
            variables_from_removed_atoms.insert(t);
        }
    }

    std::unordered_set<Term, boost::hash<Term>> important_vars_for_body_and_head;
    for (const auto &t : rule->get_effect().get_arguments()) {
        if (not t.is_object())
            important_vars_for_body_and_head.insert(t);
    }
    for (const DatalogAtom &condition : rule->get_conditions()) {
        if (condition == atom1 or condition == atom2) continue;
        for (const auto &t : condition.get_arguments()) {
            important_vars_for_body_and_head.insert(t);
        }
    }

    std::vector<Term> joining_variables;
    for (auto i = variables_from_removed_atoms.begin(); i != variables_from_removed_atoms.end(); i++) {
        if (important_vars_for_body_and_head.find(*i) != important_vars_for_body_and_head.end())
            joining_variables.push_back(*i);
    }
    return Arguments(std::move(joining_variables));
}

JoinCost compute_join_cost(const std::unique_ptr<RuleBase> &rule, const DatalogAtom &atom1, const DatalogAtom &atom2) {
    std::unordered_set<Term, boost::hash<Term>> free_atom1;
    for (const auto &t : atom1.get_arguments()) {
        if (not t.is_object()) {
            free_atom1.insert(t);
        }
    }

    std::unordered_set<Term, boost::hash<Term>> free_atom2;
    for (const auto &t : atom2.get_arguments()) {
        if (not t.is_object()) {
            free_atom2.insert(t);
        }
    }

    int arity_atom1 = free_atom1.size();
    int arity_atom2 = free_atom2.size();

    int new_arity = compute_joining_variables(rule, atom1, atom2).size();
    int max_arity = std::max(arity_atom1, arity_atom2);
    int min_arity = std::min(arity_atom1, arity_atom2);
    return JoinCost(new_arity, max_arity, min_arity);
}

}

#endif //SEARCH_DATALOG_TRANSFORMATIONS_GREEDY_JOIN_H_
