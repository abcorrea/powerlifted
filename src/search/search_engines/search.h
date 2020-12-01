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


    bool is_useful_operator(
        const Task &task,
        const DBState &state,
        const std::map<int, std::vector<GroundAtom>> &useful_atoms,
        const std::vector<bool> &useful_nullary_atoms) {
        for (size_t j = 0; j < useful_nullary_atoms.size(); ++j) {
            if (useful_nullary_atoms[j] and state.get_nullary_atoms()[j]) {
                return true;
            }
        }
        for (auto &entry : useful_atoms) {
            size_t relation = entry.first;
            if (task.predicates[relation].isStaticPredicate())
                continue;
            for (auto &tuple : entry.second) {
                if (state.get_tuples_of_relation(relation).count(tuple) > 0)
                    return true;
            }
        }
        //cerr << "not useful" << endl;
        return false;
    }

};

#endif //SEARCH_SEARCH_H
