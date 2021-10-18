#ifndef SEARCH_SUCCESSOR_GENERATORS_HYPERTREE_DECOMPOSITION_H_
#define SEARCH_SUCCESSOR_GENERATORS_HYPERTREE_DECOMPOSITION_H_

#include "generic_join_successor.h"

#include "../database/table.h"

#include <cassert>

class HTEdge {
    int parent;
    int child;
public:
    HTEdge(int u, int v) : parent(u), child(v) {}

    int get_parent() const {return parent;}
    int get_child() const {return child;}
};


class HTNode {
    std::vector<int> indices;
public:
    HTNode(const std::vector<int> &indices) : indices(indices) {}

    int get_first() const {
        assert(!indices.empty());
        return indices[0];
    }

    const std::vector<int> &get_indices() const {
        return indices;
    }

    int get_ith_index(size_t i) const {
        assert(indices.size() > i && i >= 0);
        return indices[i];
    }

};


class Hypertree {

    /*
     * Nodes of the hypertree are ordered from root (index 0) to leaves in infix order (Davide M. Longo)
     */
    std::vector<HTNode> nodes;
    std::vector<HTEdge> edges;
public:
    Hypertree() {};

    void add_node(const std::vector<int> &indices) {
        nodes.emplace_back(indices);
    }

    void add_edge(int u, int v) {
        // u is the parent, v is the children
        edges.emplace_back(u, v);
    }

    size_t get_number_of_nodes() const {
        return nodes.size();
    }

    const std::vector<HTNode> &get_nodes() const {
        return nodes;
    }

    const std::vector<HTEdge> &get_edges() const {
        return edges;
    }

    const HTEdge &get_edge(size_t i) const {
        assert(edges.size() > i && i >= 0);
        return edges[i];
    }

};


class HypertreeDecompositionSuccessor : public GenericJoinSuccessor {
    // Each hypertree is ordered from root to leaves
    std::vector<Hypertree> hypertrees;
public:
    explicit HypertreeDecompositionSuccessor(const Task &task);
    Table instantiate(const ActionSchema &action,
                      const DBState &state) final;

};

#endif //SEARCH_SUCCESSOR_GENERATORS_HYPERTREE_DECOMPOSITION_H_
