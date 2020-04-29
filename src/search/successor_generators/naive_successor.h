#ifndef SEARCH_NAIVE_SUCCESSOR_H
#define SEARCH_NAIVE_SUCCESSOR_H

#include "generic_join_successor.h"

#include <cstdlib>
#include <ctime>

// TODO Why does this class exist?
class NaiveSuccessorGenerator : public GenericJoinSuccessor {
public:
    explicit NaiveSuccessorGenerator(const Task &task) : GenericJoinSuccessor(task) {}

    /**
    * @see generic_join_successor.h
    */
    std::vector<Table>
    parse_precond_into_join_program(const std::vector<Atom> &precond,
                                    const DBState &state) final;
};


#endif //SEARCH_NAIVE_SUCCESSOR_H
