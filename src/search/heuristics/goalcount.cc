
#include "goalcount.h"
#include "../task.h"

#include <cassert>

int Goalcount::compute_heuristic(const DBState &s, const Task &task) {
    /*
     * First, loop over all nullary atoms.  Then loop over all arguments in the
     * goal condition and checks which ones are satisfied or not.
     *
     */
    const auto& nullary_atoms = s.get_nullary_atoms();
    int h = compute_unreached_nullary_atoms(task.get_goal().positive_nullary_goals,
                                            task.get_goal().negative_nullary_goals,
                                            nullary_atoms);

    for (const AtomicGoal &atomicGoal : task.get_goal().goal) {
        assert(atomicGoal.get_predicate_index()==
            s.get_relations()[atomicGoal.get_predicate_index()].predicate_symbol);
        h += atom_not_satisfied(s, atomicGoal);
    }
    return h;
}


bool Goalcount::atom_not_satisfied(const DBState &s,
                                   const AtomicGoal &atomicGoal) const {
    const auto &tuples = s.get_relations()[atomicGoal.get_predicate_index()].tuples;
    const auto it = tuples.find(atomicGoal.get_arguments());
    const auto end = tuples.end();
    return (!atomicGoal.is_negated() && it==end) || (atomicGoal.is_negated() && it!=end);
}

int
Goalcount::compute_unreached_nullary_atoms(const std::unordered_set<int> &positive,
                                           const std::unordered_set<int> &negative,
                                           const std::vector<bool> &nullary_atoms) {
    int h = 0;
    for (int pred : positive) {
        if (!nullary_atoms[pred]) {
            h++;
        }
    }
    for (int pred : negative) {
        if (nullary_atoms[pred]) {
            h++;
        }
    }
    return h;
}
