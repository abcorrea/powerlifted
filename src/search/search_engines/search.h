#ifndef SEARCH_SEARCH_H
#define SEARCH_SEARCH_H

#include "../search_statistics.h"
#include "../utils/system.h"
#include <utility>
#include <vector>


// Forward declarations
class SuccessorGenerator;
class Heuristic;
class Task;


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

class SearchBase {
public:
    SearchBase() = default;
    virtual ~SearchBase() = default;

    virtual utils::ExitCode search(const Task &task,
                       SuccessorGenerator &generator,
                       Heuristic &heuristic) = 0;

    virtual void print_statistics() const = 0;

protected:

    SearchStatistics statistics;

};

#endif //SEARCH_SEARCH_H
