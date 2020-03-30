#ifndef SEARCH_SEARCH_H
#define SEARCH_SEARCH_H

#include "../action_schema.h"
#include "../action.h"
#include "../state.h"
#include "../state_packer.h"
#include "../structures.h"
#include "../task.h"
#include "../successor_generators/successor_generator.h"
#include "../heuristics/heuristic.h"
#include "../utils/segmented_vector.h"

#include <utility>
#include <vector>

#define SOLVED 0
#define NOT_SOLVED 1
#define DEBUG_GRACEFUL_EXIT 0

class Node {
 public:
  Node(int g, int h, size_t id) : g(g), h(h), id(id) {}

  int g;
  int h;
  size_t id;
};

struct NodeComparison {
  bool operator()(const Node &n, const Node &m) const {
    if (n.h!=m.h) return n.h > m.h;
    else return n.g > m.g;
  }
};

class Search {

 public:
  Search() = default;

  int getNumberExploredStates() const;

  int getNumberGeneratedStates() const;

  virtual const int search(const Task &task,
                           SuccessorGenerator *generator,
                           Heuristic &heuristic) const = 0;

  bool is_goal(const State &state, const GoalCondition &goal) const;

  static void extract_plan(
      segmented_vector::SegmentedVector<pair<int, Action>> &cheapest_parent,
      PackedState state,
      unordered_map<PackedState, int, PackedStateHash> &visited,
      segmented_vector::SegmentedVector<PackedState> &index_to_state,
      const StatePacker &packer, const Task &task);

  std::vector<Action> plan;
  void print_no_solution_found(clock_t timer_start) const;
  void print_goal_found(
      const Task &task,
      const SuccessorGenerator *generator,
      clock_t timer_start,
      const StatePacker &state_packer,
      int generations_last_jump,
      segmented_vector::SegmentedVector<pair<int, Action>> &cheapest_parent,
      segmented_vector::SegmentedVector<PackedState> &index_to_state,
      unordered_map<PackedState, int, PackedStateHash> &visited,
      const State &state) const;
 private:
  int number_explored_states = 0;
  int number_generated_states = 0;

};

#endif //SEARCH_SEARCH_H
