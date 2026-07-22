#ifndef SEARCH_AXIOM_H
#define SEARCH_AXIOM_H

#include "atom.h"
#include "structures.h"

#include <string>
#include <utility>
#include <vector>

/**
 * @brief An axiom defining (part of) a derived predicate.
 *
 * The head is the derived predicate applied to parameters 0, ...,
 * get_head_arity() - 1. The remaining parameters are existentially
 * quantified variables occurring only in the body. The body is a
 * conjunction of positive atoms plus (in)equality literals; it is split
 * the same way action schema preconditions are:
 *
 * @var body: positive body atoms with at least one argument, excluding '='
 * literals. Arguments reference parameters ('p') or constants ('c').
 * @var equalities: literals over the '=' predicate (equalities and
 * inequalities), applied as filters during evaluation.
 * @var positive_nullary_body: indices of nullary predicates that occur
 * (positively) in the body.
 *
 * Axioms are evaluated stratum by stratum; get_stratum() returns the
 * stratum of the head predicate as computed by the translator.
 */
class Axiom {
    std::string name;
    int index;
    int head_predicate;
    int stratum;
    std::vector<Parameter> parameters;
    int num_head_parameters;
    std::vector<Atom> body;
    std::vector<Atom> equalities;
    std::vector<int> positive_nullary_body;

public:
    Axiom(std::string name,
          int index,
          int head_predicate,
          int stratum,
          std::vector<Parameter> parameters,
          int num_head_parameters,
          std::vector<Atom> body,
          std::vector<Atom> equalities,
          std::vector<int> positive_nullary_body)
        : name(std::move(name)),
          index(index),
          head_predicate(head_predicate),
          stratum(stratum),
          parameters(std::move(parameters)),
          num_head_parameters(num_head_parameters),
          body(std::move(body)),
          equalities(std::move(equalities)),
          positive_nullary_body(std::move(positive_nullary_body)) {}

    const std::string &get_name() const { return name; }

    int get_index() const { return index; }

    int get_head_predicate() const { return head_predicate; }

    int get_stratum() const { return stratum; }

    const std::vector<Parameter> &get_parameters() const { return parameters; }

    int get_head_arity() const { return num_head_parameters; }

    const std::vector<Atom> &get_body() const { return body; }

    const std::vector<Atom> &get_equalities() const { return equalities; }

    const std::vector<int> &get_positive_nullary_body() const {
        return positive_nullary_body;
    }
};

#endif // SEARCH_AXIOM_H
