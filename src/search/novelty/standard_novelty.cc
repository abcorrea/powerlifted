#include "standard_novelty.h"

using namespace std;

int StandardNovelty::compute_novelty_k1(const Task &task, const DBState &state) {
    number_unsatisfied_goals = compute_unsatisfied_goals(task, state);
    if (number_unsatisfied_goals == 0) {
        return 0;
    }
    int idx = number_unsatisfied_goals - 1;

    auto &achieved_atoms_in_layer = achieved_atoms[idx];

    bool has_novel_atom = false;
    for (const Relation &relation : state.get_relations()) {
        int pred_symbol_idx = relation.predicate_symbol;
        for (const GroundAtom& tuple : relation.tuples) {
            bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k1(pred_symbol_idx, tuple);
            if (is_new)
                has_novel_atom = true;
        }
    }

    const vector<bool>& nullary_atoms = state.get_nullary_atoms();
    for (size_t i = 0; i < nullary_atoms.size(); ++i) {
        if (nullary_atoms[i]) {
            bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k1(i, GroundAtom());
            if (is_new)
                has_novel_atom = true;
        }
    }

    if (has_novel_atom)
        return 1;
    else
        return NOT_NOVEL;

}


int StandardNovelty::compute_novelty_k2(const Task &task, const DBState &state) {
    int novelty = compute_novelty_k1(task, state);
    int idx = number_unsatisfied_goals - 1;

    if (novelty == 0) {
        // Goal state
        return 0;
    }

    bool has_k1_novelty = (novelty == 1);

    auto &achieved_atoms_in_layer = achieved_atoms[idx];

    // TODO Refactor
    for (const Relation &r1 : state.get_relations()) {
        int pred_symbol_idx1 = r1.predicate_symbol;
        for (const GroundAtom &t1 : r1.tuples) {
            for (const Relation &r2 : state.get_relations()) {
                int pred_symbol_idx2 = r2.predicate_symbol;
                for (const GroundAtom &t2 : r2.tuples) {
                    bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(pred_symbol_idx1,
                                                                                   pred_symbol_idx2,
                                                                                   t1,
                                                                                   t2);
                    if (is_new and !has_k1_novelty)
                        novelty = 2;
                }
            }
        }
    }

    const vector<bool>& nullary_atoms = state.get_nullary_atoms();
    for (size_t i = 0; i < nullary_atoms.size(); ++i) {
        if (nullary_atoms[i]) {
            int pred_symbol_idx1 = i;
            for (const Relation &r2 : state.get_relations()) {
                int pred_symbol_idx2 = r2.predicate_symbol;
                for (const GroundAtom &t2 : r2.tuples) {
                    bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(pred_symbol_idx1,
                                                                                   pred_symbol_idx2,
                                                                               GroundAtom(),
                                                                                   t2);
                    if (is_new and !has_k1_novelty)
                        novelty = 2;
                }
            }
        }
    }

    return novelty;
}


int StandardNovelty::compute_unsatisfied_goals(const Task &task, const DBState &state) {
    return gc.compute_heuristic(state, task);
}