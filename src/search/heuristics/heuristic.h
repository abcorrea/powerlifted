#ifndef SEARCH_HEURISTIC_H
#define SEARCH_HEURISTIC_H

#include "../structures.h"
#include "../task.h"

#include <iostream>
#include <map>

class DBState;
class Task;

class Heuristic {
protected:
    std::map<int, std::vector<GroundAtom>> useful_atoms;
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

    void _print_useful_atoms(const Task &task) {
        for (size_t j = 0; j < useful_nullary_atoms.size(); ++j) {
            if (useful_nullary_atoms[j])
                std::cout << task.predicates[j].getName() << ", ";
        }
        for (auto &entry : useful_atoms) {
            std::string relation_name = task.predicates[entry.first].getName();
            for (auto &tuple : entry.second) {
                std::cout << relation_name << "(";
                for (auto obj : tuple) {
                    std::cout << task.objects[obj].getName() << ",";
                }
                std::cout << "), ";
            }
        }
    }
};

#endif //SEARCH_HEURISTIC_H
