#ifndef SEARCH_SUCCESSOR_GENERATOR_H
#define SEARCH_SUCCESSOR_GENERATOR_H


#include <vector>
#include "../state.h"
#include "../action_schema.h"

class SuccessorGenerator {
public:

    SuccessorGenerator() = default;

    std::vector<State> generate_successors(std::vector<ActionSchema> actions, State state);

};


#endif //SEARCH_SUCCESSOR_GENERATOR_H
