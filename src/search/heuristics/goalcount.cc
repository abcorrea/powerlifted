//
// Created by blaas on 04.07.19.
//

#include "goalcount.h"
#include "../goal_condition.h"
#include "../task.h"

int Goalcount::compute_heuristic(const State &s, const Task &task) {
    int h = 0;
    for (const AtomicGoal &atomicGoal : task.goal.goal) {
        int goal_predicate = atomicGoal.predicate;
        Relation relation_at_goal_predicate = s.relations[goal_predicate];
        assert (goal_predicate == relation_at_goal_predicate.predicate_symbol);
        if (!atomicGoal.negated) {
            // Positive goal
            if (find(relation_at_goal_predicate.tuples.begin(), relation_at_goal_predicate.tuples.end(),
                    atomicGoal.args) == relation_at_goal_predicate.tuples.end()) {
                h++;
            }
        }
        else {
            // Negative goal
            if (find(relation_at_goal_predicate.tuples.begin(), relation_at_goal_predicate.tuples.end(),
                     atomicGoal.args) != relation_at_goal_predicate.tuples.end()) {
                h++;
            }
        }
    }
    return h;
}
