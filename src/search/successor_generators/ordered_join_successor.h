#ifndef SEARCH_ORDERED_JOIN_SUCCESSOR_H
#define SEARCH_ORDERED_JOIN_SUCCESSOR_H


#include "../task.h"
#include "../action.h"
#include "../database/table.h"
#include "successor_generator.h"


/*
 *
 * This class implements the successor generator using a full join ordered by the arity of the predicates.
 * The order is from the predicate with lowest arity to the one with largest.
 *
 *
 */

class OrderedJoinSuccessorGenerator : public SuccessorGenerator {
public:
    explicit OrderedJoinSuccessorGenerator(const Task &task) : SuccessorGenerator(task) {};

    std::vector<std::pair<State, Action>> generate_successors(const std::vector<ActionSchema> &actions, const State &state,
                                                              const StaticInformation &staticInformation) override;

    std::vector<std::vector<int>> obj_per_type; // position I is a list of object indices of type I

    Table instantiate(const ActionSchema &action, const State &state,
                      const StaticInformation &staticInformation) override;

    std::vector<Table>
    parse_precond_into_join_program(const std::vector<Atom> &precond,
                                    const State &state,
                                    const StaticInformation &staticInformation) override;
};


#endif //SEARCH_ORDERED_JOIN_SUCCESSOR_H
