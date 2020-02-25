#include <queue>
#include <stack>
#include <iostream>

#include "full_reducer_successor_generator.h"
#include "../database/semi_join.h"
#include "../database/hash_join.h"
#include "../database/hash_semi_join.h"

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
        int cont = 0;
        for (const Atom &p : action.getPrecondition()) {
            if (p.negated or p.arguments.empty()) {
                // We ignore negated preconditions and nullary predicates
                continue;
            }
            set<int> args;
            bool has_free_variables = false;
            for (Argument arg : p.arguments) {
                // We create one node for each argument of the atom
                int node;
                if (arg.constant)
                    // Constants can be ignored because we project over them
                    continue;
                has_free_variables = true;
                node = arg.index;

                args.insert(node);
                if (find(hypernodes.begin(), hypernodes.end(), node) == hypernodes.end()) {
                    node_index[node] = hypernodes.size();
                    node_counter[node] = 1;
                    hypernodes.push_back(node);
                }
                else {
                    node_counter[node] = node_counter[node] + 1;
                }
            }
            if (!args.empty() and has_free_variables) {
                edge_to_precond[hyperedges.size()] = cont; // map ith-precondition to a given edge
                hyperedges.emplace_back(args.begin(), args.end());
            }
            else {
                // If all args of a preconditions are constant, we check it first
                missing_precond.push_back(cont);
            }
            ++cont;
        }
        // Corner case: one relation
        if (hyperedges.size() <= 1) {
            if (!hyperedges.empty())
                full_join_order[action.getIndex()].push_back(0);
            continue;
        }

        /*
         * GYO algorithm.
         */
        bool has_ear = true;
        stack<pair<int,int>> full_reducer_back;
        vector<bool> removed(hyperedges.size(), false);
        while (has_ear) {
            has_ear = false;
            int ear = -1;
            int in_favor = -1;
            for (int i = 0; i < hyperedges.size() and !has_ear; ++i) {
                if (removed[i]) {
                    continue;
                }
                for (int j = 0; j < hyperedges.size() and !has_ear; ++j) {
                    if (removed[j] or i == j) {
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
                            node_counter[n] = node_counter[n]-1;
                        }
                    }
                }
                if (has_ear) {
                    assert (ear != -1 and in_favor != -1);
                    removed[ear] =true;
                    full_reducer_order[action.getIndex()].emplace_back(edge_to_precond[ear], edge_to_precond[in_favor]);
                    full_reducer_back.emplace(edge_to_precond[in_favor], edge_to_precond[ear]);
                    full_join_order[action.getIndex()].push_back(edge_to_precond[ear]);
                }
            }
        }
        while (!full_reducer_back.empty()) {
            pair<int, int> p = full_reducer_back.top();
            full_reducer_order[action.getIndex()].push_back(p);
            full_reducer_back.pop();
        }
        // Add all hyperedges that were not removed to the join. If it is acyclic, there is only left.
        for (int k : missing_precond)
            full_join_order[action.getIndex()].push_back(k);
        reverse(full_join_order[action.getIndex()].begin(), full_join_order[action.getIndex()].end());
        int not_removed_counter = 0;
        for (auto && k : removed) {
            if (!k) {
                ++not_removed_counter;
            }
        }
        if (not_removed_counter == 1) {
            for (int k = 0; k < removed.size(); ++k) {
                if (!removed[k]) {
                    full_join_order[action.getIndex()].push_back(edge_to_precond[k]);
                }
            }
            //cout << "Action " << action.getName() << " is acyclic.\n";
            acyclic_vec[action.getIndex()] = true;
        }
        else {
            priority_queue<pair<int,int>> q;
            full_join_order[action.getIndex()].clear();
            full_join_order[action.getIndex()].reserve(removed.size()+missing_precond.size());
            for (int k = 0; k < removed.size(); ++k) {
                q.emplace(hyperedges[k].size(), edge_to_precond[k]);
            }
            for (int k = 0; k < missing_precond.size(); ++k) {
                q.emplace(action.getPrecondition()[k].arguments.size(), missing_precond[k]);
            }
            while (!q.empty()) {
                int p = q.top().second;
                full_join_order[action.getIndex()].push_back(p);
                q.pop();
            }
            //cout << "Action " << action.getName() << " is cyclic.\n";
            acyclic_vec[action.getIndex()] = false;
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
Table FullReducerSuccessorGenerator::instantiate(const ActionSchema &action, const State &state,
                                                 const StaticInformation &staticInformation) {

    /*
     *  We need to parse precond first
     */

    clock_t time = clock();

    vector<vector<int>> instantiations;
    const vector<Parameter> &params = action.getParameters();
    vector<Atom> precond;

    if (params.empty()) {
        return Table();
    }

    for (const Atom &p : action.getPrecondition()) {
        // Ignoring negative preconditions when instantiating
        if (!p.negated and !p.arguments.empty()) {
            precond.push_back((p));
        }
    }

    assert (!precond.empty());

    vector<Table> tables = parse_precond_into_join_program(precond, state, staticInformation, action.getIndex());
    if (tables.size() != full_join_order[action.getIndex()].size()) {
        // This means that the projection over the constants completely eliminated one table,
        // we can return no instantiation.
        if (!acyclic_vec[action.getIndex()])
            cyclic_time += double(clock() - time) / CLOCKS_PER_SEC;
        return Table();
    }
    assert (!tables.empty());
    for (const pair<int,int> &sj : full_reducer_order[action.getIndex()]) {
        int s = semi_join(tables[sj.first], tables[sj.second]);
        if (s == 0) {
            if (!acyclic_vec[action.getIndex()])
                cyclic_time += double(clock() - time) / CLOCKS_PER_SEC;
            return Table();
        }
    }

    Table &working_table = tables[full_join_order[action.getIndex()][0]];
    for (int i = 1; i < full_join_order[action.getIndex()].size(); ++i) {
        hash_join(working_table, tables[full_join_order[action.getIndex()][i]]);
        if (working_table.tuples.size() > largest_intermediate_relation)
            largest_intermediate_relation = working_table.tuples.size();

        // Filter out equalities
        for (const pair<int, int> &ineq : action.getInequalities()) {
            auto it_1 = find(working_table.tuple_index.begin(),
                             working_table.tuple_index.end(),
                             ineq.first);
            auto it_2 = find(working_table.tuple_index.begin(),
                             working_table.tuple_index.end(),
                             ineq.second);
            int index1 = distance(working_table.tuple_index.begin(), it_1);
            int index2 = distance(working_table.tuple_index.begin(), it_2);
            if (it_1 != working_table.tuple_index.end() and it_2 != working_table.tuple_index.end()) {
                vector<vector<int>> to_remove;
                for (auto && t : working_table.tuples) {
                    if (t[index1] == t[index2])
                        to_remove.push_back(t);
                }
                for (auto &&t : to_remove) {
                    working_table.tuples.erase(t);
                }
            }
        }
        if (working_table.tuples.empty()) {
            if (!acyclic_vec[action.getIndex()])
                cyclic_time += double(clock() - time) / CLOCKS_PER_SEC;
            return working_table;
        }
    }

    return working_table;
}
