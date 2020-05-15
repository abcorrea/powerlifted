#ifndef SEARCH_RANDOM_SUCCESSOR_H
#define SEARCH_RANDOM_SUCCESSOR_H

#include "generic_join_successor.h"

#include <random>

/**
 * This class implements a successor generator based on a randomly ordered
 * join program.
 */
class RandomSuccessorGenerator : public GenericJoinSuccessor {
    std::default_random_engine rng;

public:
    explicit RandomSuccessorGenerator(const Task &task, unsigned seed);

    // @see generic_join_successor.h
    bool parse_precond_into_join_program(const PrecompiledActionData &adata,
                                         const DBState &state,
                                         std::vector<Table>& tables) override;
};


#endif //SEARCH_RANDOM_SUCCESSOR_H
