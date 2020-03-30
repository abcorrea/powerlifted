#include "breadth_first_search.h"

#include <iostream>
#include <queue>
#include <vector>

using namespace std;

const int BreadthFirstSearch::search(const Task &task,
                                     SuccessorGenerator *generator,
                                     Heuristic &heuristic) const {
  /*
   * Simple Breadth first search
   */

  cout << "Starting breadth first search" << endl;
  clock_t timer_start = clock();
  StatePacker state_packer(task);

  size_t state_counter = 0;
  int generations = 0;
  int generations_last_jump = 0;
  int expansions = 0;
  queue<Node> q; // Queue has Node structures
  segmented_vector::SegmentedVector<pair<int, Action>> cheapest_parent;
  segmented_vector::SegmentedVector<PackedState> index_to_state;
  unordered_map<PackedState, int, PackedStateHash> visited;

  index_to_state.push_back(state_packer.pack_state(task.initial_state));
  cheapest_parent.push_back(make_pair(-1, Action(-1, vector<int>())));

  int g_layer = 0;

  q.emplace(0, 0, state_counter);
  visited[state_packer.pack_state(task.initial_state)] = state_counter++;

  if (task.is_goal(task.initial_state, task.goal)) {
    cout << "Initial state is a goal" << endl;
    print_goal_found(
        task, generator, timer_start, state_packer, generations_last_jump,
        cheapest_parent, index_to_state,visited, task.initial_state);
    return SOLVED;
  }
  while (not q.empty()) {
    Node head = q.front();
    size_t next = head.id;
    int g = head.g;
    expansions++;
    q.pop();
    if (g_layer < g) {
      generations_last_jump = generations;
      g_layer = g;
    }
    assert (index_to_state.size() >= next);
    State state = state_packer.unpack_state(index_to_state[next]);
    vector<pair<State, Action>> successors =
        generator->generate_successors(task.actions, state, task.static_info);

    generations += successors.size();
    int init_state_succ = 0;
    for (const pair<State, Action> &successor : successors) {
      const State &s = successor.first;
      const PackedState &packed = state_packer.pack_state(s);
      const Action &a = successor.second;
      if (visited.find(packed)==visited.end()) {
        init_state_succ++;
        cheapest_parent.push_back(make_pair(next, a));
        q.emplace(g + 1, 0, state_counter);
        index_to_state.push_back(packed);
        visited[packed] = state_counter;
        if (task.is_goal(s, task.goal)) {
          print_goal_found(
              task, generator, timer_start, state_packer, generations_last_jump,
              cheapest_parent,index_to_state,visited, s);
          return SOLVED;
        }
        state_counter++;
      }
    }
  }

  print_no_solution_found(timer_start);

  return NOT_SOLVED;
}
