#include "goalcount.h"

#include "../goal_condition.h"
#include "../task.h"

int Goalcount::compute_heuristic(const State &s, const Task &task) {
    /*
     * First, loop over all nullary atoms.  Then loop over all arguments in the
     * goal condition and checks which ones are satisfied or not.
     *
     */
    int h = compute_reached_nullary_atoms(task.goal.positive_nullary_goals,
                                          s.nullary_atoms);
    h += compute_reached_nullary_atoms(task.goal.negative_nullary_goals,
                                       s.nullary_atoms);;

    for (const AtomicGoal &atomicGoal : task.goal.goal) {
        assert(atomicGoal.predicate==
            s.relations[atomicGoal.predicate].predicate_symbol);
        h += atom_not_satisfied(s, atomicGoal);
    }
    return h;
}


int Goalcount::atom_not_satisfied(const State &s,
                                  const AtomicGoal &atomicGoal) const {
    if (!atomicGoal.negated) {
        // Positive goal
        if (find(s.relations[atomicGoal.predicate].tuples.begin(),
                 s.relations[atomicGoal.predicate].tuples.end(),
                 atomicGoal.args)==
            s.relations[atomicGoal.predicate].tuples.end()) {
            return 1;
        }
    }
    else {
        // Negative goal
        if (find(s.relations[atomicGoal.predicate].tuples.begin(),
                 s.relations[atomicGoal.predicate].tuples.end(),
                 atomicGoal.args)!=
            s.relations[atomicGoal.predicate].tuples.end()) {
            return 1;
        }
    }
    return 0;
}

int
Goalcount::compute_reached_nullary_atoms(const std::unordered_set<int> &indices,
                                         const std::vector<bool> &nullary_atoms) {
    int h = 0;
    for (int pred : indices) {
        if (!nullary_atoms[pred])
            h++;
    }
    return h;
}
