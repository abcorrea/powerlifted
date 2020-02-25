#ifndef SEARCH_YANNAKAKIS_H
#define SEARCH_YANNAKAKIS_H


#include "generic_join_successor.h"

class YannakakisSuccessorGenerator : public GenericJoinSuccessor {
public:
    /**
   * @see yannakakis.cc
   * @param task
   */
    explicit YannakakisSuccessorGenerator(const Task &task);
    Table instantiate(const ActionSchema &action, const State &state, const StaticInformation &staticInformation) final;

private:
    std::vector<std::vector<std::pair<int, int>>> full_reducer_order;
    std::vector<std::vector<std::pair<int, int>>> join_tree_order;
    std::vector<std::vector<int>> remaining_join;
    std::vector<bool> acyclic_vec;
    std::vector<std::unordered_set<int>> distinguished_variables;
};


#endif //SEARCH_YANNAKAKIS_H
