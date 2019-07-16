#ifndef SEARCH_NAIVE_SUCCESSOR_H
#define SEARCH_NAIVE_SUCCESSOR_H

#include "generic_join_successor.h"

class NaiveSuccessorGenerator : public GenericJoinSuccessor {
public:
    explicit NaiveSuccessorGenerator(const Task &task) : GenericJoinSuccessor(task) {}

    std::vector<Table>
    parse_precond_into_join_program(const std::vector<Atom> &precond,
                                    const State &state,
                                    const StaticInformation &staticInformation,
                                    int action_index) override;

};


#endif //SEARCH_NAIVE_SUCCESSOR_H
