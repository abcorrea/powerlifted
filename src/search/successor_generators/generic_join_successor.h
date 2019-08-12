#ifndef SEARCH_GENERIC_JOIN_SUCCESSOR_H
#define SEARCH_GENERIC_JOIN_SUCCESSOR_H

#include <cstdlib>
#include <ctime>

#include <random>

#include "successor_generator.h"

/*
 * This class is not a successor generator per se. It just contain most of the common functions
 * used over all the join successor generators.
 *
 * However, the join implementation is in ../database/join.{h,cc}
 */

class GenericJoinSuccessor : public SuccessorGenerator {
public:
    explicit GenericJoinSuccessor(const Task &task) : SuccessorGenerator(task) {
        srand(time(nullptr));
    }

    std::vector<std::vector<int>> obj_per_type; // position I is a list of object indices of type I

    Table instantiate(const ActionSchema &action, const State &state,
                      const StaticInformation &staticInformation) override;

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
