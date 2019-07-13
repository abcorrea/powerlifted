#ifndef SEARCH_ORDERED_JOIN_SUCCESSOR_H
#define SEARCH_ORDERED_JOIN_SUCCESSOR_H


#include "../task.h"
#include "../action.h"
#include "../database/table.h"
#include "generic_join_successor.h"


/*
 * This class implements the successor generator using a full join ordered by the arity of the predicates.
 * The order is from the predicate with lowest arity to the one with largest.
 *
 */

class OrderedJoinSuccessorGenerator : public GenericJoinSuccessor {
public:
    explicit OrderedJoinSuccessorGenerator(const Task &task) : GenericJoinSuccessor(task) {};

    std::vector<Table>
    parse_precond_into_join_program(const std::vector<Atom> &precond,
                                    const State &state,
                                    const StaticInformation &staticInformation) override;
};


#endif //SEARCH_ORDERED_JOIN_SUCCESSOR_H
