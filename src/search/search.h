#ifndef SEARCH_SEARCH_H
#define SEARCH_SEARCH_H

#include <utility>
#include <vector>

#include "action_schema.h"
#include "action.h"
#include "state.h"
#include "structures.h"
#include "task.h"
#include "successor_generators/successor_generator.h"

class Search {

public:
    Search() = default;

    int getNumberExploredStates() const;

    int getNumberGeneratedStates() const;

    const vector<Action> &search(const Task &task, SuccessorGenerator generator) const;

private:
    int number_explored_states = 0;
    int number_generated_states = 0;
    std::vector<Action> plan;

    bool is_goal(const State& state) const;

    bool is_goal(const State &state, const GoalCondition &goal) const;
};


#endif //SEARCH_SEARCH_H
