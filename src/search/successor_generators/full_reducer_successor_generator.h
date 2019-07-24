//
// Created by gutob on 14.07.2019.
//

#ifndef SEARCH_FULL_REDUCER_SUCCESSOR_GENERATOR_H
#define SEARCH_FULL_REDUCER_SUCCESSOR_GENERATOR_H


#include "generic_join_successor.h"

class FullReducerSuccessorGenerator : public GenericJoinSuccessor {
public:
    explicit FullReducerSuccessorGenerator(const Task &task);

    const std::vector<std::pair<State, Action>> &generate_successors(const std::vector<ActionSchema> &actions,
                                                              const State &state,
                                                              const StaticInformation &staticInformation) final;

    Table instantiate(const ActionSchema &action, const State &state,
                      const StaticInformation &staticInformation) override;

    vector<Table> parse_precond_into_join_program(const vector<Atom> &precond,
                                                  const State &state,
                                                  const StaticInformation &staticInformation,
                                                  int action_index) override;


private:
    std::vector<std::vector<std::pair<int, int>>> full_reducer_order;
    std::vector<std::vector<int>> full_join_order;

    };


#endif //SEARCH_FULL_REDUCER_SUCCESSOR_GENERATOR_H
