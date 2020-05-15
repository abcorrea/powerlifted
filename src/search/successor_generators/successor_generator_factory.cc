
#include "successor_generator_factory.h"

#include "full_reducer_successor_generator.h"
#include "naive_successor.h"
#include "ordered_join_successor.h"
#include "random_successor.h"
#include "yannakakis.h"

#include "../database/table.h"

#include <iostream>

#include <boost/algorithm/string.hpp>

SuccessorGenerator *SuccessorGeneratorFactory::create(const std::string &method,
                                                      unsigned seed,
                                                      Task &task)
{
    std::cout << "Creating successor generator factory..." << std::endl;
    if (boost::iequals(method, "join")) {
        return new NaiveSuccessorGenerator(task);
    }
    else if (boost::iequals(method, "full_reducer")) {
        return new FullReducerSuccessorGenerator(task);
    }
    else if (boost::iequals(method, "inverse_ordered_join")) {
        return new OrderedJoinSuccessorGenerator<InverseOrderTable>(task);
    }
    else if (boost::iequals(method, "ordered_join")) {
        return new OrderedJoinSuccessorGenerator<OrderTable>(task);
    }
    else if (boost::iequals(method, "random_join")) {
        return new RandomSuccessorGenerator(task, seed);
    }
    else if (boost::iequals(method, "yannakakis")) {
        return new YannakakisSuccessorGenerator(task);
    }
    else {
        std::cerr << "Invalid successor generator method \"" << method << "\"" << std::endl;
        exit(-1);
    }
}
