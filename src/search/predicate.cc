#include "predicate.h"


int Predicate::getArity() const {
    return arity;
}

const std::vector<int> &Predicate::getTypes() const {
    return types;
}

bool Predicate::isStaticPredicate() const {
    return static_predicate;
}

const std::string &Predicate::get_name() const {
    return name;
}

