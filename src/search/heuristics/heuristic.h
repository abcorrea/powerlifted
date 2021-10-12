#ifndef SEARCH_HEURISTIC_H
#define SEARCH_HEURISTIC_H

#include "../structures.h"
#include "../task.h"

#include <iostream>
#include <map>

const int UNSOLVABLE_STATE = std::numeric_limits<int>::max();

class DBState;
class Task;

class Heuristic {
protected:
    // TODO This could be a simpler std::vector<std::vector<GroundAtom>>
    std::map<int, std::vector<GroundAtom>> useful_atoms;

protected:
    std::vector<bool> useful_nullary_atoms;

public:
    virtual ~Heuristic() = default;

    /**
     * @brief Virtual implementation of a heuristic function
     * @param s: State being evaluated
     * @param task: Planning task
     * @return Heuristic value
     */
    virtual int compute_heuristic(const DBState &s, const Task &task) = 0;

    const std::map<int, std::vector<GroundAtom>> &get_useful_atoms() const {
        return useful_atoms;
    }

    const std::vector<bool> &get_useful_nullary_atoms() const {
        return useful_nullary_atoms;
    }

    void _print_useful_atoms(const Task &task) {
        for (auto &entry : useful_atoms) {
            std::string relation_name = task.predicates[entry.first].get_name();
            for (auto &tuple : entry.second) {
                std::cout << relation_name << "(";
                for (auto obj : tuple) {
                    std::cout << task.objects[obj].get_name() << ",";
                }
                std::cout << "), ";
            }
        }
        std::cout << std::endl;
    }
};

#endif //SEARCH_HEURISTIC_H
