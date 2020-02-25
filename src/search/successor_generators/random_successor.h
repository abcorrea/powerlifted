#ifndef SEARCH_RANDOM_SUCCESSOR_H
#define SEARCH_RANDOM_SUCCESSOR_H

#include <cstdlib>
#include <ctime>

#include "generic_join_successor.h"

/**
 * This class implements a successor generator based on a randomly ordered
 * join program.
 */
class RandomSuccessorGenerator : public GenericJoinSuccessor {
public:
    explicit RandomSuccessorGenerator(const Task &task) : GenericJoinSuccessor(task) {
        srand(time(nullptr));
    }

  /**
  * @see generic_join_successor.h
  */
  std::vector<Table>
    parse_precond_into_join_program(const std::vector<Atom> &precond,
                                    const State &state,
                                    const StaticInformation &staticInformation,
                                    int action_index) final;
};


#endif //SEARCH_RANDOM_SUCCESSOR_H
