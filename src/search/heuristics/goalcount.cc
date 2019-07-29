//
// Created by blaas on 04.07.19.
//

#include "goalcount.h"
#include "../goal_condition.h"
#include "../task.h"

int Goalcount::compute_heuristic(const State &s, const Task &task) {
    int h = 0;
    for (const AtomicGoal &atomicGoal : task.goal.goal) {
        assert (atomicGoal.predicate == s.relations[atomicGoal.predicate].predicate_symbol);
        if (task.predicates[s.relations[atomicGoal.predicate].predicate_symbol].isStaticPredicate())
            continue;
        if (!atomicGoal.negated) {
            // Positive goal
            if (find(s.relations[atomicGoal.predicate].tuples.begin(), s.relations[atomicGoal.predicate].tuples.end(),
                    atomicGoal.args) == s.relations[atomicGoal.predicate].tuples.end()) {
                h++;
            }
        }
        else {
            // Negative goal
            if (find(s.relations[atomicGoal.predicate].tuples.begin(), s.relations[atomicGoal.predicate].tuples.end(),
                     atomicGoal.args) != s.relations[atomicGoal.predicate].tuples.end()) {
                h++;
            }
        }
    }
    return h;
}
