#ifndef SEARCH_SUCCESSOR_GENERATOR_FACTORY_H
#define SEARCH_SUCCESSOR_GENERATOR_FACTORY_H

#include <iostream>
#include <boost/algorithm/string.hpp>

#include "full_reducer_successor_generator.h"
#include "naive_successor.h"
#include "ordered_join_successor.h"
#include "inverse_ordered_join_successor.h"

class SuccessorGeneratorFactory {
public:
    static SuccessorGenerator *new_generator(const std::string & method, const Task &task) {
        std::cout << "Creating successor generator factory..." << std::endl;
        if (boost::iequals(method, "join")) {
            return new NaiveSuccessorGenerator(task);
        }
        else if (boost::iequals(method, "ordered_join")) {
            return new OrderedJoinSuccessorGenerator(task);
        }
        else if (boost::iequals(method, "inverse_ordered_join")) {
            return new InverseOrderedJoinSuccessorGenerator(task);
        }
        else if (boost::iequals(method, "full_reducer")) {
            return new FullReducerSuccessorGenerator(task);
        }
        else {
            return nullptr;
        }
    }
};


#endif //SEARCH_SUCCESSOR_GENERATOR_FACTORY_H
