#ifndef SEARCH_SEARCH_H
#define SEARCH_SEARCH_H

#include "search_space.h"
#include "utils.h"

#include "../search_statistics.h"
#include "../utils/system.h"

#include "../states/extensional_states.h"
#include "../states/sparse_states.h"
#include "../successor_generators/successor_generator.h"
#include "../task.h"

#include <utility>
#include <vector>
#include <iostream>
#include <queue>

// Forward declarations
class SuccessorGenerator;
class Heuristic;
class Task;


class SearchBase {
public:
    SearchBase() = default;
    virtual ~SearchBase() = default;

    virtual utils::ExitCode search(const Task &task,
                       SuccessorGenerator &generator,
                       Heuristic &heuristic) = 0;

    virtual void print_statistics() const = 0;

    template <class PackedStateT>
    bool check_goal(const Task &task,
                    const SuccessorGenerator &generator,
                    clock_t timer_start,
                    const DBState &state,
                    const SearchNode &node,
                    const SearchSpace<PackedStateT> &space) const {
        if (!task.is_goal(state)) return false;

        print_goal_found(generator, timer_start);
        auto plan = space.extract_plan(node);
        print_plan(plan, task);
        return true;
    }

protected:

    SearchStatistics statistics;

};

#endif //SEARCH_SEARCH_H
