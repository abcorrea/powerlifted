#ifndef SEARCH_YANNAKAKIS_H
#define SEARCH_YANNAKAKIS_H

#include "generic_join_successor.h"

class JoinTree;

class YannakakisSuccessorGenerator : public GenericJoinSuccessor {
 public:
  /**
 * @see yannakakis.cc
 * @param task
 */
  explicit YannakakisSuccessorGenerator(const Task &task);
  Table instantiate(const ActionSchema &action,
                    const DBState &state) final;

 private:
  std::vector<std::vector<std::pair<int, int>>> full_reducer_order;

  std::vector<std::unordered_set<int>> distinguished_variables;

  std::vector<std::vector<int>> remaining_join;

  std::vector<JoinTree> join_trees;

  void get_distinguished_variables(const ActionSchema &action);
};

class JoinTree {
    std::vector<std::pair<int, int>> join_tree_order;

public:
    JoinTree() = default;

    void add_node(int i, int j) {
        join_tree_order.emplace_back(i, j);
    }

    const std::vector<std::pair<int, int>> &get_order() const {
        return join_tree_order;
    }

};


#endif //SEARCH_YANNAKAKIS_H
