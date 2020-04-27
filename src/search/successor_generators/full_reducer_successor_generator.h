#ifndef SEARCH_FULL_REDUCER_SUCCESSOR_GENERATOR_H
#define SEARCH_FULL_REDUCER_SUCCESSOR_GENERATOR_H

#include "generic_join_successor.h"

class FullReducerSuccessorGenerator : public GenericJoinSuccessor {
public:
  /**
   * @see full_reducer_successor_generator.cc
   * @param task
   */
    explicit FullReducerSuccessorGenerator(const Task &task);

    Table instantiate(const ActionSchema &action, const DBState &state,
                      const StaticInformation &staticInformation) override;

private:
    std::vector<std::vector<std::pair<int, int>>> full_reducer_order;
    std::vector<std::vector<int>> full_join_order;
    std::vector<bool> acyclic_vec;

};


#endif //SEARCH_FULL_REDUCER_SUCCESSOR_GENERATOR_H
