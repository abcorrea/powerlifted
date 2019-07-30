#include <utility>

#ifndef SEARCH_GOAL_CONDITION_H
#define SEARCH_GOAL_CONDITION_H

#include <utility>
#include <unordered_set>
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
    std::unordered_set<int> positive_nullary_goals;
    std::unordered_set<int> negative_nullary_goals;

    GoalCondition() = default;

    explicit GoalCondition(std::vector<AtomicGoal> goal,
                           std::unordered_set<int> positive_nullary_goals,
                           std::unordered_set<int> negative_nullary_goals) :
            goal(std::move(goal)),
            positive_nullary_goals(std::move(positive_nullary_goals)),
            negative_nullary_goals(std::move(negative_nullary_goals)) {

    }
};


#endif //SEARCH_GOAL_CONDITION_H
