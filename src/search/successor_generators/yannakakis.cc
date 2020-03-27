#include "yannakakis.h"

#include "../database/hash_join.h"
#include "../database/project.h"
#include "../database/semi_join.h"

#include <stack>
#include <queue>


/**
 *
 * @attention This code has a lot of duplication from full_reducer_successor_generator.cc
 *
 * @details The only difference between the Yannakakis and the Full reducer
 * successor generators is how the complete join sequence is computed after
 * the full reducer. Here, we find the same order that would be done by
 * Yannakakis' algorithm, based on the join tree.
 *
 * @see full_reducer_successor_generator.cc
 *
 * @param task
 */
YannakakisSuccessorGenerator::YannakakisSuccessorGenerator(const Task &task) : GenericJoinSuccessor(task) {
   /*
     * Apply GYO algorithm for every action schema to check whether it has acyclic precondition/
     *
     * A lot of duplication from Full reducer successor generator.
     */
    full_reducer_order.resize(task.actions.size());
    // Join tree order is the order in which the project-join is performed in a join tree.
    // Every entry is a pair of nodes, where the first one is the child of the second.
    // The idea is that we can compute a join tree from the GYO algorithm in a bottom-up
    // style based on the ear removal order. E.g., if we remove E in favor of F, then
    // E is a child of F in the tree.
    join_tree_order.resize(task.actions.size());
    remaining_join.resize(task.actions.size());
    acyclic_vec.resize(task.actions.size());
    distinguished_variables.resize(task.actions.size());
    for (const ActionSchema &action : task.actions) {
        vector<int> hypernodes;
        vector<set<int>> hyperedges;
        vector<int> missing_precond;
        map<int, int> node_index;
        map<int, int> node_counter;
        map<int, int> edge_to_precond;
        map<int, int> precond_to_size;
        int cont = 0;
        for (const Atom &eff : action.get_effects()) {
            for (Argument arg : eff.arguments) {
                if (!arg.constant)
                    distinguished_variables[action.get_index()].insert(arg.index);
            }
        }
        for (const Atom &p : action.get_precondition()) {
            if (p.negated or p.arguments.empty()) {
                continue;
            }
            set<int> args;
            bool has_free_variables = false;
            for (Argument arg : p.arguments) {
                // We parse constants to negative numbers so they're uniquely identified
                int node;
                if (arg.constant)
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

        /*
         * GYO algorithm.
         * We probably should have a better method to order cyclic precond
         */
        bool has_ear = true;
        stack<pair<int,int>> full_reducer_back;
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
                    full_reducer_order[action.get_index()].emplace_back(edge_to_precond[ear], edge_to_precond[in_favor]);
                    full_reducer_back.emplace(edge_to_precond[in_favor], edge_to_precond[ear]);
                    join_tree_order[action.get_index()].emplace_back(edge_to_precond[ear], edge_to_precond[in_favor]);
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
            remaining_join[action.get_index()].push_back(k);
        int not_removed_counter = 0;
        for (auto && k : removed) {
            if (!k) {
                ++not_removed_counter;
            }
        }
        if (not_removed_counter == 1) {
            /*
             * We need to add the root of every component and join them.
             * But since we considered all components when computing the full reducer,
             * there is only one.
             */
            for (size_t k = 0; k < removed.size(); ++k) {
                if (!removed[k]) {
                    remaining_join[action.get_index()].push_back(edge_to_precond[k]);
                }
            }
            //cout << "Action " << action.get_name() << " is acyclic.\n";
            acyclic_vec[action.get_index()] = true;
        }
        else {
            priority_queue<pair<int,int>> q;
            remaining_join[action.get_index()].clear();
            remaining_join[action.get_index()].reserve(removed.size()+missing_precond.size());
            for (size_t k = 0; k < missing_precond.size(); ++k) {
                q.emplace(action.get_precondition()[k].arguments.size(), missing_precond[k]);
            }
            for (size_t k = 0; k < removed.size(); ++k) {
                q.emplace(hyperedges[k].size(), edge_to_precond[k]);
            }
            while (!q.empty()) {
                int p = q.top().second;
                remaining_join[action.get_index()].push_back(p);
                q.pop();
            }
            //cout << "Action " << action.get_name() << " is cyclic.\n";
            acyclic_vec[action.get_index()] = false;
        }
    }



}

/**
 *
 * Instantiate a given action at a given state using Yannakakis' algorithm.
 *
 *
 * @attention Partially duplicated from full_reducer_successor_generator.cc
 *
 * @details The process here is the same as the full reducer. However, as we
 * process the join tree (instead of a simple join sequence), we need to apply
 * the projection operation as defined by Yannakakis' algorithm. See Ullman's
 * book or Correa et al. ICAPS 2020 for details.
 *
 * @see database/hash_join.h
 * @see database/semi_join.h
 * @see full_reducer_successor_generator.cc
 *
 * @param action Action schema currently being isntantiated
 * @param state State used as database
 * @param staticInformation  Static predicates of the task
 * @return
 */
Table YannakakisSuccessorGenerator::instantiate(const ActionSchema &action, const State &state,
                                                 const StaticInformation &staticInformation) {

    /*
     *  We need to parse precond first
     */

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

    vector<Table> tables = parse_precond_into_join_program(precond, state, staticInformation,
                                                           action.get_index());
    if (tables.size() != precond.size()) {
        // This means that the projection over the constants completely eliminated one table,
        // we can return no instantiation.
        return Table();
    }
    assert (!tables.empty());
    for (const pair<int,int> &sj : full_reducer_order[action.get_index()]) {
        // We do not check inequalities here. Should we?
        size_t s = semi_join(tables[sj.first], tables[sj.second]);
        if (s == 0) {
            return Table();
        }
    }


    for (const auto &j : join_tree_order[action.get_index()]) {
        unordered_set<int> project_over;
        for (auto x : tables[j.second].tuple_index) {
            project_over.insert(x);
        }
        for (auto x : tables[j.first].tuple_index) {
            if (distinguished_variables[action.get_index()].count(x) > 0) {
                project_over.insert(x);
            }
        }
        Table &working_table = tables[j.second];
        hash_join(working_table, tables[j.first]);
        if (working_table.tuples.size() > largest_intermediate_relation)
            largest_intermediate_relation = working_table.tuples.size();
        // Filter out equalities
        for (const pair<int, int> &ineq : action.get_inequalities()) {
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
        // Project must be after removal of inequality constraints, otherwise we might keep only the tuple violating
        // some inequality.
        // This is the main difference to the full reducer successor generator
        project(working_table, project_over);
        if (working_table.tuples.empty()) {
            return working_table;
        }
    }

    // For the case where the action schema is cyclic
    Table &working_table = tables[remaining_join[action.get_index()][0]];
    for (size_t i = 1; i < remaining_join[action.get_index()].size(); ++i) {
        hash_join(working_table, tables[remaining_join[action.get_index()][i]]);
        if (working_table.tuples.size() > largest_intermediate_relation)
            largest_intermediate_relation = working_table.tuples.size();
        // Filter out equalities
        for (const pair<int, int> &ineq : action.get_inequalities()) {
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
            return working_table;
        }
    }

    return working_table;
}
