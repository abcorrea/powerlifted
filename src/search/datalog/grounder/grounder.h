#ifndef GROUNDER_GROUNDERS_GROUNDER_H_
#define GROUNDER_GROUNDERS_GROUNDER_H_

#include "../datalog.h"

namespace datalog {

class Grounder {

public:
    Grounder() = default;

    virtual ~Grounder() = default;

    virtual int ground(Datalog &lp, std::vector<Fact> &state_facts, int goal_predicate) = 0;

    virtual void print_statistics(const Datalog &lp) = 0;
};

}

#endif //GROUNDER_GROUNDERS_GROUNDER_H_
