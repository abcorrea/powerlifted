#include <utility>

#ifndef SEARCH_GOAL_CONDITION_H
#define SEARCH_GOAL_CONDITION_H

#include <utility>
#include <vector>

using namespace std;

struct AtomicGoal {
    int predicate;
    std::vector<int> args;
    bool negated;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
    AtomicGoal(int predicate, std::vector<int> args, bool negated) : predicate(predicate), args(std::move(args)),
                                                                     negated (negated) { }
#pragma clang diagnostic pop
};

class GoalCondition {
public:
    std::vector<AtomicGoal> goal;

    GoalCondition() = default;

    explicit GoalCondition(std::vector<AtomicGoal> goal) : goal(std::move(goal)) {

    }
};


#endif //SEARCH_GOAL_CONDITION_H
