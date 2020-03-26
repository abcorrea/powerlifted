#ifndef SEARCH_GENERIC_JOIN_SUCCESSOR_H
#define SEARCH_GENERIC_JOIN_SUCCESSOR_H

#include "successor_generator.h"

#include <cstdlib>
#include <ctime>
#include <random>

/**
 * This class is not a successor generator per se. It just contain most of the common functions
 * used over all the join successor generators.
 *
 * @details This contains the main functions for successor generators based on
 * join of preconditions .The main function of this class is the instantiate
 * function, which performs the join program itself. If used, this class
 * orders the join program using the same order as the PDDL file. This behavior
 * is defined in the function parse_precond_into_join_program. Classes
 * extending the GenericJoinSuccessor class usually replace this function for
 * something more elaborated.
 *
 * @see database/join.cc
 */

class GenericJoinSuccessor : public SuccessorGenerator {
public:
    explicit GenericJoinSuccessor(const Task &task) : SuccessorGenerator(task) {}

    std::vector<std::vector<int>> obj_per_type; // position I is a list of object indices of type I

    Table instantiate(const ActionSchema &action, const State &state,
                      const StaticInformation &staticInformation) override;

    /**
    * Parse the state and the atom preconditions into a set of tables
    * to perform the join-program more easily.
    *
    * We first obtain all indices in the precondition that are constants.
    * Then, we create the table applying the projection over the arguments
    * that satisfy the instantiation of the constants. There are two cases
    * for the projection:
    *    1. The table comes from the static information; or
    *    2. The table comes directly from the current state.
    *
    * @param precond: list of atoms in the preconditions
    * @param state: state being evaluated
    * @param staticInformation: static predicates of the task
    * @param action_index: index of the action being instantiated
    * @return Table containing the tuples satisfying the query established by the precondition
    */
    std::vector<Table> parse_precond_into_join_program(const std::vector<Atom> &precond,
                                                       const State &state,
                                                       const StaticInformation &staticInformation,
                                                       int action_index) override;

protected:
    const void get_indices_and_constants_in_preconditions(std::vector<int> &indices,
                                                          std::vector<int> &constants,
                                                          const Atom &a);

    const void project_tuples(const State &s,
                              const Atom &a,
                              unordered_set<GroundAtom, TupleHash> &tuples,
                              const std::vector<int> &constants);
};


#endif //SEARCH_GENERIC_JOIN_SUCCESSOR_H
