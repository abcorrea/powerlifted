//
// Created by gutob on 14.07.2019.
//

#include <queue>
#include <stack>
#include <iostream>

#include "full_reducer_successor_generator.h"
#include "../database/semi_join.h"
#include "../database/hash_join.h"

using namespace std;

FullReducerSuccessorGenerator::FullReducerSuccessorGenerator(const Task &task) :
        GenericJoinSuccessor(task) {
    /*
     * Apply GYO algorithm for every action schema to check whether it has acyclic precondition
     * TODO add unary precond at the end of the full join
     */
    full_reducer_order.resize(task.actions.size());
    full_join_order.resize(task.actions.size());
    for (const ActionSchema &action : task.actions) {
        vector<int> hypernodes;
        vector<set<int>> hyperedges;
        map<int, int> node_index;
        map<int, int> node_counter;
        map<int, int> edge_to_precond;
        map<int, int> precond_to_size;
        int cont = 0;
        for (const Atom &p : action.getPrecondition()) {
            if (p.negated) {
                continue;
            }
            if (p.negated or p.tuples.size() == 0) {
                continue;
            }
            set<int> args;
            for (Argument arg : p.tuples) {
                // We parse constants to negative numbers so they're unique identified
                int node;
                if (arg.constant) {
                    node = -1 * arg.index;
                } else {
                    node = arg.index;
                }
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
            edge_to_precond[hyperedges.size()] = cont++; // map ith-precondition to a given edge
            hyperedges.emplace_back(args.begin(), args.end());
        }
        // Corner case: one relation
        if (hyperedges.size() <= 1) {
            if (!hyperedges.empty())
                full_join_order[action.getIndex()].push_back(0);
            return;
        }

        /*
         * GYO algorithm.
         * We probably should have a better method to order cyclic precond
         */
        bool has_ear = true;
        stack<pair<int,int>> full_reducer_back;
        vector<bool> removed(hyperedges.size(), false);
        while (has_ear and !hyperedges.empty()) {
            has_ear = false;
            int ear = -1;
            int in_favor = -1;
            for (int i = 0; i < hyperedges.size()-1 and !has_ear; ++i) {
                if (removed[i]) {
                    continue;
                }
                for (int j = i+1; j < hyperedges.size() and !has_ear; ++j) {
                    if (removed[j]) {
                        continue;
                    }
                    set<int> diff;
                    set_difference(hyperedges[i].begin(), hyperedges[i].end(),
                                   hyperedges[j].begin(), hyperedges[j].end(),
                                   inserter(diff, diff.end()));
                    // Contained only in the first hyperedge, then it is an ear
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
                        for (int n : diff) {
                            node_counter[n]--;
                        }
                    }
                }
                if (has_ear) {
                    assert (ear != -1 and in_favor != -1);
                    removed[ear] =true;
                    full_reducer_order[action.getIndex()].emplace_back(edge_to_precond[ear], edge_to_precond[in_favor]);
                    full_reducer_back.emplace(edge_to_precond[in_favor], edge_to_precond[ear]);
                    full_join_order[action.getIndex()].push_back(ear);
                }
            }
        }
        while (!full_reducer_back.empty()) {
            pair<int, int> p = full_reducer_back.top();
            full_reducer_order[action.getIndex()].push_back(p);
            full_reducer_back.pop();
        }
        // Add all hyperedges that were not removed to the join. If it is acyclic, there is only left.
        reverse(full_join_order[action.getIndex()].begin(), full_join_order[action.getIndex()].end());
        for (int k = 0; k < removed.size(); ++k) {
            if (!removed[k])
                full_join_order[action.getIndex()].push_back(edge_to_precond[k]);
        }
    }
}

std::vector<std::pair<State, Action>>
FullReducerSuccessorGenerator::generate_successors(const std::vector<ActionSchema> &actions, const State &state,
                                                   const StaticInformation &staticInformation) {

    // Duplicate code from generic join implementation
    vector<pair<State, Action>> successors;

    for (const ActionSchema &action : actions) {
        Table instantiations = instantiate(action, state, staticInformation);
        /*
         * See comment in generic_join_successor.cc
         */
        if (instantiations.tuples.empty()) {
            continue;
            // TODO case where action is pre grounded (no parameters)
        } else {
            for (const vector<int> &tuple : instantiations.tuples) {
                // TODO support for !=
                // TODO test case with constants (should work?)
                vector<Relation> new_relation(state.relations);
                for (const Atom &eff : action.getEffects()) {
                    const GroundAtom &ground_atom = tuple_to_atom(tuple, instantiations.tuple_index, eff);
                    assert (eff.predicate_symbol == new_relation[eff.predicate_symbol].predicate_symbol);
                    if (eff.negated) {
                        // Remove from relation
                        new_relation[eff.predicate_symbol].tuples.erase(remove(
                                new_relation[eff.predicate_symbol].tuples.begin(),
                                new_relation[eff.predicate_symbol].tuples.end(), ground_atom),
                                                                        new_relation[eff.predicate_symbol].tuples.end());
                    } else {
                        if (find(new_relation[eff.predicate_symbol].tuples.begin(),
                                 new_relation[eff.predicate_symbol].tuples.end(), ground_atom)
                            == new_relation[eff.predicate_symbol].tuples.end()) {
                            // If ground atom is not in the state, we add it
                            new_relation[eff.predicate_symbol].tuples.push_back(ground_atom);
                        }
                    }
                }
                successors.emplace_back(new_relation, Action(action.getIndex(), tuple));
            }
        }
    }
    return successors;
}

Table FullReducerSuccessorGenerator::instantiate(const ActionSchema &action, const State &state,
                                                 const StaticInformation &staticInformation) {

    /*
     *  We need to parse precond first
     */

    vector<vector<int>> instantiations;
    const vector<Parameter> &params = action.getParameters();
    vector<Atom> precond;
    for (const Atom &p : action.getPrecondition()) {
        // Ignoring negative preconditions when instantiating
        if (not p.negated) {
            precond.push_back((p));
        }
    }

    if (params.empty()) {
        return Table();
    }

    assert (!precond.empty());

    vector<Table> tables = parse_precond_into_join_program(precond, state, staticInformation, action.getIndex());
    assert(tables.size() == full_join_order[action.getIndex()].size());
    assert (!tables.empty());
    for (const pair<int,int> &sj : full_reducer_order[action.getIndex()]) {
        // We do not check inequalities here. Should we?
        semi_join(tables[sj.first], tables[sj.second]);
    }

    Table working_table = tables[full_join_order[action.getIndex()][0]];
    for (int i = 1; i < full_join_order[action.getIndex()].size(); ++i) {
        join(working_table, tables[full_join_order[action.getIndex()][i]]);
        // Filter out equalities
        for (const pair<int, int> ineq : action.getInequalities()) {
            auto it_1 = find(working_table.tuple_index.begin(),
                             working_table.tuple_index.end(),
                             ineq.first);
            auto it_2 = find(working_table.tuple_index.begin(),
                             working_table.tuple_index.end(),
                             ineq.second);
            if (it_1 != working_table.tuple_index.end() and it_2 != working_table.tuple_index.end()) {
                // Loop over all tuples and remove duplicates
                // This is a late removal but there is no easy way to do it before
                int index1 = distance(working_table.tuple_index.begin(), it_1);
                int index2 = distance(working_table.tuple_index.begin(), it_2);
                int cont = 0;
                vector<int> equal_tuples;
                for (vector<int> tuple : working_table.tuples) {
                    if (tuple[index1] == tuple[index2]) {
                        equal_tuples.push_back(cont);
                    }
                    cont++;
                }
                reverse(equal_tuples.begin(), equal_tuples.end());
                for (int n : equal_tuples) {
                    working_table.tuples.erase(working_table.tuples.begin() + n);
                }
            }
        }
        if (working_table.tuples.empty()) {
            return working_table;
        }
    }

    return working_table;
}

vector<Table> FullReducerSuccessorGenerator::parse_precond_into_join_program(const vector<Atom> &precond,
                                                                             const State &state,
                                                                             const StaticInformation &staticInformation,
                                                                             int action_index) {
    /*
     * We first parse the state and the atom preconditions into a set of tables
     * to perform the join-program more easily.
     */
    vector<Table> parsed_tables;//(precond.size());
    parsed_tables.reserve(precond.size());
    for (const Atom &a : precond) {
        vector<int> indices;
        for (Argument arg : a.tuples) {
            indices.push_back(arg.index);
        }
        if (!staticInformation.relations[a.predicate_symbol].tuples.empty()) {
            // If this predicate has information in the static information table,
            // then it must be a static predicate
            parsed_tables.emplace_back(staticInformation.relations[a.predicate_symbol].tuples, indices);
        } else {
            // If this predicate does not have information in the static information table,
            // then it must be a fluent
            parsed_tables.emplace_back(state.relations[a.predicate_symbol].tuples, indices);
        }
    }
    return parsed_tables;
}
