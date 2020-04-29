
#include "action.h"

#include <iostream>

using namespace std;

ostream &operator<<(ostream &os, const LiftedOperatorId& id) {
    os << "op" << id.index;
    return os;
}

const LiftedOperatorId LiftedOperatorId::no_operator = LiftedOperatorId(-1, {});