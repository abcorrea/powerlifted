
#include "goalcount.h"
#include "../task.h"

#include <cassert>

int Goalcount::compute_heuristic(const DBState &s, const Task &task) {
    /*
     * First, loop over all nullary atoms.  Then loop over all arguments in the
     * goal condition and checks which ones are satisfied or not.
     *
     */
    auto nullary_atoms = s.get_nullary_atoms();
    int h = compute_reached_nullary_atoms(task.goal.positive_nullary_goals,
                                          nullary_atoms);
    h += compute_reached_nullary_atoms(task.goal.negative_nullary_goals,
                                       nullary_atoms);;

    for (const AtomicGoal &atomicGoal : task.goal.goal) {
        assert(atomicGoal.predicate==
            s.get_relations()[atomicGoal.predicate].predicate_symbol);
        h += atom_not_satisfied(s, atomicGoal);
    }
    return h;
}


int Goalcount::atom_not_satisfied(const DBState &s,
                                  const AtomicGoal &atomicGoal) const {
    const auto it = s.get_relations()[atomicGoal.predicate].tuples.find(atomicGoal.args);
    const auto end = s.get_relations()[atomicGoal.predicate].tuples.end();
    if ((!atomicGoal.negated && it == end) || (atomicGoal.negated && it != end)) {
        return 1;
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
