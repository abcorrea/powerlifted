#include "greedy_best_first_search.h"

#include "search.h"

#include "../states/sparse_states.h"
#include "../utils/segmented_vector.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <vector>

using namespace std;

template<class PackedStateT>
int GreedyBestFirstSearch<PackedStateT>::search(const Task &task,
                                        SuccessorGenerator *generator,
                                        Heuristic &heuristic) {
  cout << "Starting greedy best first search" << endl;
  clock_t timer_start = clock();
  SparseStatePacker state_packer(task);


  priority_queue<Node, vector<Node>, NodeComparison>
      q; // Queue has Node structures
  segmented_vector::SegmentedVector<pair<int, Action>> cheapest_parent;

  segmented_vector::SegmentedVector<SparsePackedState> index_to_state;
  unordered_map<SparsePackedState, int, PackedStateHash> visited;

  segmented_vector::SegmentedVector<int> shortest_distance;

  index_to_state.push_back(state_packer.pack_state(task.initial_state));
  cheapest_parent.push_back(make_pair(-1, Action(-1, vector<int>())));

  this->heuristic_layer =
      heuristic.compute_heuristic(task.initial_state, task) + 1;
  cout << "Initial heuristic value " << this->heuristic_layer << endl;

  q.emplace(0,
            heuristic.compute_heuristic(task.initial_state, task),
            this->state_counter);
  shortest_distance.push_back(0);
  visited[state_packer.pack_state(task.initial_state)] = this->state_counter++;

  if (task.is_goal(task.initial_state, task.goal)) {
    cout << "Initial state is a goal" << endl;
    print_goal_found(
        task, generator, timer_start, state_packer, this->generations_last_jump,
        cheapest_parent, index_to_state,visited, task.initial_state);
    return SOLVED;
  }

  while (not q.empty()) {
    Node head = q.top();
    size_t next = head.id;
    int h = head.h;
    int g = head.g;
    q.pop();
    if (this->g_layer < g) {
      this->generations_last_jump = this->generations;
      this->g_layer = g;
    }
    if (g > shortest_distance[next]) {
      continue;
    }
    if (h < this->heuristic_layer) {
        this->heuristic_layer = h;
      cout << "New heuristic value expanded: h=" << h <<
           " [this->state_counter: " << this->state_counter << ", this->generations: " << this->generations
           << ", time: " << double(clock() - timer_start)/CLOCKS_PER_SEC << "]"
           << '\n';
    }
    assert (index_to_state.size() >= next);
    State state = state_packer.unpack_state(index_to_state[next]);
    if (task.is_goal(state, task.goal)) {
      print_goal_found(
          task, generator, timer_start, state_packer, this->generations_last_jump,
          cheapest_parent,index_to_state,visited, state);
      return SOLVED;
    }
    vector<pair<State, Action>> successors =
        generator->generate_successors(task.actions, state, task.static_info);
    this->generations += successors.size();
    int init_state_succ = 0;
    for (const pair<State, Action> &successor : successors) {
      const State &s = successor.first;
      const SparsePackedState packed = state_packer.pack_state(s);
      const Action &a = successor.second;
      int dist = g + task.actions[a.index].get_cost();
      int new_h = heuristic.compute_heuristic(s, task);
      pair<unordered_map<SparsePackedState, int, PackedStateHash>::iterator, bool>
          try_to_insert = visited.insert(make_pair(packed, this->state_counter));
      if (try_to_insert.second) {
        // Inserted for the first time in the map
        init_state_succ++;
        cheapest_parent.push_back(make_pair(next, a));
        q.emplace(dist, new_h, this->state_counter);
        shortest_distance.push_back(dist);
        index_to_state.push_back(packed);
        this->state_counter++;
      } else {
        int index = visited[packed];
        if (dist < shortest_distance[index]) {
          cheapest_parent[index] = make_pair(next, a);
          q.emplace(dist, new_h, index);
          shortest_distance[index] = dist;
        }
      }
    }
  }

  this->print_no_solution_found(timer_start);

  return NOT_SOLVED;

}


// explicit template instantiations
template class GreedyBestFirstSearch<SparsePackedState>;