
#include "successor_generator_factory.h"

#include "clique_successor_generator.h"
#include "full_reducer_successor_generator.h"
#include "naive_successor.h"
#include "ordered_join_successor.h"
#include "random_successor.h"
#include "yannakakis.h"

#include "../database/table.h"

#include <iostream>

#include "../utils/system.h"
#include "../utils/string_utils.h"

SuccessorGenerator *
SuccessorGeneratorFactory::create(const std::string &method, unsigned seed, Task &task)
{
    std::cout << "Creating successor generator factory..." << std::endl;
    if (utils::iequals(method, "join")) {
        return new NaiveSuccessorGenerator(task);
    }
    else if (utils::iequals(method, "full_reducer")) {
        return new FullReducerSuccessorGenerator(task);
    }
    else if (utils::iequals(method, "inverse_ordered_join")) {
        return new OrderedJoinSuccessorGenerator<InverseOrderTable>(task);
    }
    else if (utils::iequals(method, "ordered_join")) {
        return new OrderedJoinSuccessorGenerator<OrderTable>(task);
    }
    else if (utils::iequals(method, "random_join")) {
        return new RandomSuccessorGenerator(task, seed);
    }
    else if (utils::iequals(method, "yannakakis")) {
        return new YannakakisSuccessorGenerator(task);
    }
    else if (utils::iequals(method, "clique_bk")) {
        return new CliqueSuccessorGenerator(task, BronKerboschFirst);
    }
    else if (utils::iequals(method, "clique_kckp")) {
        return new CliqueSuccessorGenerator(task, KCliqueKPartite);
    }
    else {
        std::cerr << "Invalid successor generator method \"" << method << "\"" << std::endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }
}
