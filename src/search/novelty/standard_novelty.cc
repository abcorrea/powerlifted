#include "standard_novelty.h"

int StandardNovelty::compute_novelty(const Task &task, const DBState &state) {
    int number_unsatisfied_goals = gc.compute_heuristic(state, task);
    if (number_unsatisfied_goals == 0) {
        return GOAL;
    }
    int idx = number_unsatisfied_goals - 1;

    auto &achieved_atoms_in_layer = achieved_atoms[idx];

    bool has_novel_atom = false;
    for (const Relation &relation : state.get_relations()) {
        int pred_symbol_idx = relation.predicate_symbol;
        for (const GroundAtom& tuple : relation.tuples) {
            bool is_new = achieved_atoms_in_layer.try_to_insert(pred_symbol_idx, tuple);
            if (is_new)
                has_novel_atom = true;
        }
    }

    const std::vector<bool>& nullary_atoms = state.get_nullary_atoms();
    for (size_t i = 0; i < nullary_atoms.size(); ++i) {
        if (nullary_atoms[i]) {
            bool is_new = achieved_atoms_in_layer.try_to_insert_nullary_atom(i);
            if (is_new)
                has_novel_atom = true;
        }
    }

    if (has_novel_atom)
        return NOVEL;
    else
        return NOT_NOVEL;

}
