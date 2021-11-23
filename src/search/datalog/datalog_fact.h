#ifndef GROUNDER__FACT_H_
#define GROUNDER__FACT_H_

#include "datalog_atom.h"

#include "achievers.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/functional/hash.hpp>

namespace  datalog {

/*
 *
 * The class fact represents a ground atom that is true in the LP.
 * The arguments of the atom are represented by their index, and they refer
 * to Objects. Each Fact also has an index and a predicate name.
 *
 */
class Fact : public DatalogAtom {
    // Fact index is used to be able to refer to a specific fact in the vector of
    // facts of a LogicProgram. In this way, we only refer to the fact by its
    // index in the vector and we do not need to keep a mapping between facts
    // and indices.
    int fact_index;
    int cost;
    Achievers achievers;
public:

    static int next_fact_index;

    Fact(Arguments arguments, int predicate_index, bool new_pred) :
        DatalogAtom(std::move(arguments), predicate_index, new_pred) {
        // Every fact starts with a fact of -1 and then we set it to a proper value
        // if the fact was not previously reached.
        fact_index = -1;
        cost = 0;
    }

    Fact(Arguments arguments, int predicate_index, int cost, bool new_pred) :
        DatalogAtom(std::move(arguments), predicate_index, new_pred), cost(cost) {
        // See comment in constructor above
        fact_index = -1;
    }

    Fact(Arguments arguments, int predicate_index, int cost, Achievers achievers, bool new_pred) :
        DatalogAtom(std::move(arguments), predicate_index, new_pred), cost(cost), achievers(std::move(achievers)) {
        // See comment in constructor above
        fact_index = -1;
    }



    /*
     * The function below compares if two facts are equal. We do not care about
     * fact_index because they are not set in the point of comparison.
     */
    friend bool operator==(const Fact &a, const Fact &b) {
        return a.get_predicate_index() == b.get_predicate_index() && a.get_arguments() == b.get_arguments();
    }

    void set_fact_index() {
        fact_index = next_fact_index++;
    }

    void update_fact_index(int i) {
        fact_index = i;
    }

    int get_fact_index() const {
        return fact_index;
    }

    static int get_next_fact_index() {
        return next_fact_index;
    }

    static void reset_global_fact_index(int j) {
        next_fact_index = j;
    }

    int get_cost() const {
        return cost;
    }

    const Achievers &get_achievers() const {
        return achievers;
    }

    void update_achievers(Achievers a) {
        achievers = std::move(a);
    }

    const Achievers &get_achiever_body() const {
        return achievers;
    }

    int get_achiever_rule_index() const {
        return achievers.get_achiever_rule_index();
    }

    void set_cost(int new_cost) {
        cost = new_cost;
    }

};

}


/*
 * Hash of Facts
 *
 * TODO maybe change it for hash of atoms?
 */
template<>
struct std::hash<datalog::Fact> {
    // See comment of operator==
    std::size_t operator()(const datalog::Fact &f) const {
        std::size_t seed = boost::hash_range(f.get_arguments().begin(), f.get_arguments().end());
        boost::hash_combine(seed, f.get_predicate_index());
        return seed;
    }
};

#endif //GROUNDER__FACT_H_
