#include "hypertree_decomposition.h"

#include "../task.h"

#include "../database/hash_join.h"
#include "../database/project.h"
#include "../database/semi_join.h"

#include <cassert>
#include <fstream>
#include <iostream>

using namespace std;

HypertreeDecompositionSuccessor::HypertreeDecompositionSuccessor(const Task &task)
    : GenericJoinSuccessor(task) {

    /*
     * TODO There seems to be some error with inequalities
     */

    // TODO Make this into parameter
    ifstream infile("decompositions.out");
    int ht_width = 0;
    hypertrees.resize(task.actions.size());
    for (size_t action_idx = 0; action_idx < task.actions.size(); action_idx++) {
        int number_nodes = 0;
        infile >> number_nodes;
        for (int i = 0; i  < number_nodes; ++i) {
            int number_relations = 0;
            infile >> number_relations;
            ht_width = std::max(ht_width, number_relations);
            vector<int> relations_indices(number_relations);
            for (int j = 0; j  < number_relations; ++j)
                infile >> relations_indices[j];
            hypertrees[action_idx].add_node(relations_indices);
        }
        int number_edges = 0;
        infile >> number_edges;
        for (int i = 0; i < number_edges; ++i) {
            // u is the parent; v is the child node.
            int u, v;
            infile >> u >> v;
            hypertrees[action_idx].add_edge(u, v);
        }

        // Compute BFS order over edges
        hypertrees[action_idx].compute_bfs_order();

    }
    cout << "Finished creating hypertrees for successor generation...." << endl;
    cout << "Maximum hypertree width: " << ht_width << endl;
}


Table HypertreeDecompositionSuccessor::instantiate(const ActionSchema &action,
                                                   const DBState &state) {

    if (action.is_ground()) {
        throw std::runtime_error("Shouldn't be calling instantiate() on a ground action");
    }

    const auto& actiondata = action_data[action.get_index()];

    vector<Table> tables;
    auto res = parse_precond_into_join_program(actiondata, state, tables);
    if (!res) return Table::EMPTY_TABLE();

    assert(!tables.empty());
    assert(tables.size() == actiondata.relevant_precondition_atoms.size());

    const Hypertree &ht = hypertrees[action.get_index()];

    // Join every node using the first table as working table
    for (const auto &node : ht.get_nodes()) {
        Table &working_table = tables[node.get_first()];
        for (size_t i = 1; i < node.get_indices().size(); ++i) {
            // TODO This is the bottleneck apparently. Order relations in node in some smart way
            hash_join(working_table, tables[node.get_ith_index(i)]);
            if (working_table.tuples.empty()) {
                return Table::EMPTY_TABLE();
            }
        }
    }

    // Bottom up semi-join, needs to be in the inverted order of the hypertree
    for (int i = ht.get_edges().size() - 1; i >= 0; --i) {
        const HTEdge &edge = ht.get_edge(i);
        int parent = ht.get_node(edge.get_parent()).get_first();
        int child = ht.get_node(edge.get_child()).get_first();
        size_t s = semi_join(tables[parent], tables[child]);
        if (s==0) {
            return Table::EMPTY_TABLE();
        }
    }

    // Top down semi-join
    for (size_t i = 0; i < ht.get_edges().size(); ++i) {
        const HTEdge &edge = ht.get_edge(i);
        int parent = ht.get_node(edge.get_parent()).get_first();
        int child = ht.get_node(edge.get_child()).get_first();
        size_t s = semi_join(tables[child], tables[parent]);
        if (s==0) {
            return Table::EMPTY_TABLE();
        }
    }

    Table &working_table = tables[ht.get_node(0).get_first()];
    for (size_t i = 1; i < ht.get_number_of_nodes(); ++i) {
        size_t before = working_table.tuples.size();
        hash_join(working_table, tables[ht.get_node(i).get_first()]);
        size_t after = working_table.tuples.size();
        if (before > after) {
            cerr << "ERROR: Some tuples are not matching in the final join of action schema " << action.get_index() << endl;
            exit(0);
        }
        filter_inequalities(action, working_table);
        if (working_table.tuples.empty()) {
            return Table::EMPTY_TABLE();
        }
    }

    return working_table;
}
