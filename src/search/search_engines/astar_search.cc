#include <iostream>
#include <queue>
#include <vector>

#include "astar_search.h"

using namespace std;

const std::vector<Action> &AStarSearch::search(const Task &task,
                                               SuccessorGenerator generator,
                                               Heuristic &heuristic) const {
    /*
     * TODO implement A*
     */
    cout << "Starting A* search" << endl;
    clock_t timer_start = clock();
    return plan;
}
