#ifndef SEARCH_INVERSE_ORDERED_JOIN_SUCCESSOR_H
#define SEARCH_INVERSE_ORDERED_JOIN_SUCCESSOR_H

#include "successor_generator.h"

/*
 * This class implements a full join program ordering joins by its arity.
 * Predicates with higher arity are joined first
 */

class InverseOrderedJoinSuccessorGenerator : public SuccessorGenerator {
public:
    explicit InverseOrderedJoinSuccessorGenerator(const Task &task) : SuccessorGenerator(task) {};

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


#endif //SEARCH_INVERSE_ORDERED_JOIN_SUCCESSOR_H
