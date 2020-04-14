#include <queue>
#include <stack>

#include "full_reducer_successor_generator.h"

#include "../database/semi_join.h"
#include "../database/hash_join.h"

using namespace std;

/**
 * Creates the full reducer and already computes which action schemas are
 * acyclic or not. For the acyclic action schemas, it already computes the
 * full reducer program and the join order. For cyclic action schemas, it
 * computes the 'partial reducer'
 *
 *
 * @param task: planning task
 */
FullReducerSuccessorGenerator::FullReducerSuccessorGenerator(const Task &task) :
    GenericJoinSuccessor(task) {
  /*
   * Apply GYO algorithm for every action schema to check whether it
   * has an acyclic precondition.
   *
   * See Ullman's book for an explanation of the algorithm.
   */
  full_reducer_order.resize(task.actions.size());
  full_join_order.resize(task.actions.size());
  acyclic_vec.resize(task.actions.size());
  for (const ActionSchema &action : task.actions) {
    vector<int> hypernodes;
    vector<set<int>> hyperedges;
    vector<int> missing_precond;
    map<int, int> node_index;
    map<int, int> node_counter;
    map<int, int> edge_to_precond;
    map<int, int> precond_to_size;
    create_hypergraph(action,
                      hypernodes,
                      hyperedges,
                      missing_precond,
                      node_index,
                      node_counter,
                      edge_to_precond);

    // Corner case: one relation
    if (hyperedges.size() <= 1) {
      if (!hyperedges.empty())
        full_join_order[action.get_index()].push_back(0);
      continue;
    }

    /*
     * GYO algorithm.
     */
    bool has_ear = true;
    stack<pair<int, int>> full_reducer_back;
    vector<bool> removed(hyperedges.size(), false);
    while (has_ear) {
      has_ear = false;
      int ear = -1;
      int in_favor = -1;
      for (size_t i = 0; i < hyperedges.size() and !has_ear; ++i) {
        if (removed[i]) {
          continue;
        }
        for (size_t j = 0; j < hyperedges.size() and !has_ear; ++j) {
          if (removed[j] or i==j) {
            continue;
          }
          set<int> diff;
          // Contained only in the first hyperedge, then it is an ear
          set_difference(hyperedges[i].begin(), hyperedges[i].end(),
                         hyperedges[j].begin(), hyperedges[j].end(),
                         inserter(diff, diff.end()));
          has_ear = true;
          ear = i;
          in_favor = j;
          for (int n : diff) {
            if (node_counter[n] > 1) {
              has_ear = false;
              ear = -1;
              in_favor = -1;
            }
          }
          if (has_ear) {
            for (int n : hyperedges[i]) {
              node_counter[n] = node_counter[n] - 1;
            }
          }
        }
        if (has_ear) {
          assert (ear!=-1 and in_favor!=-1);
          removed[ear] = true;
          full_reducer_order[action.get_index()].emplace_back(edge_to_precond[ear],
                                                              edge_to_precond[in_favor]);
          full_reducer_back.emplace(edge_to_precond[in_favor],
                                    edge_to_precond[ear]);
          full_join_order[action.get_index()].push_back(edge_to_precond[ear]);
        }
      }
    }
    while (!full_reducer_back.empty()) {
      pair<int, int> p = full_reducer_back.top();
      full_reducer_order[action.get_index()].push_back(p);
      full_reducer_back.pop();
    }
    // Add all hyperedges that were not removed to the join. If it is acyclic, there is only left.
    for (int k : missing_precond)
      full_join_order[action.get_index()].push_back(k);
    reverse(full_join_order[action.get_index()].begin(),
            full_join_order[action.get_index()].end());
    int not_removed_counter = 0;
    for (auto &&k : removed) {
      if (!k) {
        ++not_removed_counter;
      }
    }
    if (not_removed_counter==1) {
      for (size_t k = 0; k < removed.size(); ++k) {
        if (!removed[k]) {
          full_join_order[action.get_index()].push_back(edge_to_precond[k]);
        }
      }
      //cout << "Action " << action.get_name() << " is acyclic.\n";
      acyclic_vec[action.get_index()] = true;
    } else {
      priority_queue<pair<int, int>> q;
      full_join_order[action.get_index()].clear();
      full_join_order[action.get_index()].reserve(
          removed.size() + missing_precond.size());
      for (size_t k = 0; k < removed.size(); ++k) {
        q.emplace(hyperedges[k].size(), edge_to_precond[k]);
      }
      for (size_t k = 0; k < missing_precond.size(); ++k) {
        q.emplace(action.get_precondition()[k].arguments.size(),
                  missing_precond[k]);
      }
      while (!q.empty()) {
        int p = q.top().second;
        full_join_order[action.get_index()].push_back(p);
        q.pop();
      }
      //cout << "Action " << action.get_name() << " is cyclic.\n";
      acyclic_vec[action.get_index()] = false;
    }
  }
}

/**
 *
 * Instantiate a given action at a given state using the full reducer method.
 *
 * @details We first get the non-negative preconditions of the action. Then, we
 * parse the state into tables with headers identifying the variables.
 * This transformation already takes into account projections. We then perform
 * the full reducer program followed by the complete join program. During the
 * join program, after each single join, we check if there are inequality
 * constraints over the variables recently joined. If there are, we filter out
 * tuples violating these constraints.
 *
 * @see database/hash_join.h
 * @see database/semi_join.h
 *
 * @param action Action schema currently being isntantiated
 * @param state State used as database
 * @param staticInformation  Static predicates of the task
 * @return
 */
Table FullReducerSuccessorGenerator::instantiate(const ActionSchema &action,
                                                 const State &state,
                                                 const StaticInformation &staticInformation) {

  /*
   *  We need to parse precond first
   */

  clock_t time = clock();

  vector<vector<int>> instantiations;
  const vector<Parameter> &params = action.get_parameters();
  vector<Atom> precond;

  if (params.empty()) {
    return Table();
  }

  for (const Atom &p : action.get_precondition()) {
    // Ignoring negative preconditions when instantiating
    if (!p.negated and !p.arguments.empty()) {
      precond.push_back((p));
    }
  }

  assert (!precond.empty());

  vector<Table> tables =
      parse_precond_into_join_program(precond, state, staticInformation,
                                      action.get_index());
  if (tables.size()!=full_join_order[action.get_index()].size()) {
    // This means that the projection over the constants completely eliminated one table,
    // we can return no instantiation.
    if (!acyclic_vec[action.get_index()])
      cyclic_time += double(clock() - time)/CLOCKS_PER_SEC;
    return Table();
  }
  assert (!tables.empty());
  for (const pair<int, int> &sj : full_reducer_order[action.get_index()]) {
    size_t s = semi_join(tables[sj.first], tables[sj.second]);
    if (s==0) {
      if (!acyclic_vec[action.get_index()])
        cyclic_time += double(clock() - time)/CLOCKS_PER_SEC;
      return Table();
    }
  }

  Table &working_table = tables[full_join_order[action.get_index()][0]];
  for (size_t i = 1; i < full_join_order[action.get_index()].size(); ++i) {
    hash_join(working_table, tables[full_join_order[action.get_index()][i]]);
    if (working_table.tuples.size() > largest_intermediate_relation)
      largest_intermediate_relation = working_table.tuples.size();
    filter_inequalities(action, working_table);
    if (working_table.tuples.empty()) {
      if (!acyclic_vec[action.get_index()])
        cyclic_time += double(clock() - time)/CLOCKS_PER_SEC;
      return working_table;
    }
  }

  return working_table;
}
