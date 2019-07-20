#ifndef SEARCH_GENERIC_JOIN_SUCCESSOR_H
#define SEARCH_GENERIC_JOIN_SUCCESSOR_H

#include "successor_generator.h"

/*
 * This class is not a successor generator per se. It just contain most of the common functions
 * used over all the join successor generators.
 *
 * However, the join implementation is in ../database/join.{h,cc}
 */

class GenericJoinSuccessor : public SuccessorGenerator {
public:
    explicit GenericJoinSuccessor(const Task &task) : SuccessorGenerator(task) {}

    const std::vector<std::pair<State, Action>> &generate_successors(const std::vector<ActionSchema> &actions,
                                                                     const State &state,
                                                                     const StaticInformation &staticInformation) override;

    std::vector<std::vector<int>> obj_per_type; // position I is a list of object indices of type I

    Table instantiate(const ActionSchema &action, const State &state,
                      const StaticInformation &staticInformation) override;

};


#endif //SEARCH_GENERIC_JOIN_SUCCESSOR_H
