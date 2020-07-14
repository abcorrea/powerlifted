#ifndef SEARCH_HEURISTIC_H
#define SEARCH_HEURISTIC_H

class DBState;
class Task;

class Heuristic {
public:
    virtual ~Heuristic() = default;

    /**
     * @brief Virtual implementation of a heuristic function
     * @param s: State being evaluated
     * @param task: Planning task
     * @return Heuristic value
     */
    virtual int compute_heuristic(const DBState &s, const Task &task) = 0;
};

#endif //SEARCH_HEURISTIC_H
