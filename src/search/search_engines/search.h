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
