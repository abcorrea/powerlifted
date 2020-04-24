#ifndef SEARCH_SUCCESSOR_GENERATOR_FACTORY_H
#define SEARCH_SUCCESSOR_GENERATOR_FACTORY_H

#include "full_reducer_successor_generator.h"
#include "inverse_ordered_join_successor.h"
#include "naive_successor.h"
#include "ordered_join_successor.h"
#include "random_successor.h"
#include "yannakakis.h"

#include <iostream>

#include <boost/algorithm/string.hpp>

class SuccessorGeneratorFactory {
public:
    static SuccessorGenerator *create(const std::string & method, const Task &task) {
        std::cout << "Creating successor generator factory..." << std::endl;
        if (boost::iequals(method, "join")) {
            return new NaiveSuccessorGenerator(task);
        }
        else if (boost::iequals(method, "full_reducer")) {
            return new FullReducerSuccessorGenerator(task);
        }
        else if (boost::iequals(method, "inverse_ordered_join")) {
            return new InverseOrderedJoinSuccessorGenerator(task);
        }
        else if (boost::iequals(method, "ordered_join")) {
            return new OrderedJoinSuccessorGenerator(task);
        }
        else if (boost::iequals(method, "random_join")) {
            return new RandomSuccessorGenerator(task);
        }
        else if (boost::iequals(method, "yannakakis")) {
            return new YannakakisSuccessorGenerator(task);
        }
        else {
            std::cerr << "Invalid successor generator method \"" << method << "\"" << std::endl;
            exit(-1);
        }
    }
};


#endif //SEARCH_SUCCESSOR_GENERATOR_FACTORY_H
