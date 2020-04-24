#ifndef SEARCH_SEARCH_H
#define SEARCH_SEARCH_H

#include <utility>
#include <vector>

#define SOLVED 0
#define NOT_SOLVED 1


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

    virtual int search(const Task &task,
                       SuccessorGenerator *generator,
                       Heuristic &heuristic) = 0;

protected:
    size_t state_counter{};
    int generations{};
    int generations_last_jump{};
    int g_layer{};
    int heuristic_layer{};
};

#endif //SEARCH_SEARCH_H
