#ifndef GROUNDER_GROUNDERS_GROUNDER_H_
#define GROUNDER_GROUNDERS_GROUNDER_H_

#include "../logic_program.h"

namespace lifted_heuristic {

class Grounder {
public:
    Grounder() = default;

    virtual ~Grounder() = default;

    virtual int ground(LogicProgram &lp, int goal_predicate) = 0;
};

}

#endif //GROUNDER_GROUNDERS_GROUNDER_H_
