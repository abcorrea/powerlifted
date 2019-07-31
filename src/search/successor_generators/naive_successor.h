#ifndef SEARCH_NAIVE_SUCCESSOR_H
#define SEARCH_NAIVE_SUCCESSOR_H

#include "generic_join_successor.h"

class NaiveSuccessorGenerator : public GenericJoinSuccessor {
public:
    explicit NaiveSuccessorGenerator(const Task &task) : GenericJoinSuccessor(task) {}

};


#endif //SEARCH_NAIVE_SUCCESSOR_H
