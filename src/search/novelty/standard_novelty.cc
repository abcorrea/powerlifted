#include "standard_novelty.h"

using namespace std;

int StandardNovelty::compute_novelty_k1(const Task &task,
                                        const DBState &state,
                                        int number_unsatisfied_goals) {
    if (number_unsatisfied_goals == 0) {
        return GOAL_STATE;
    }
    int idx = number_unsatisfied_goals - 1;

    auto &achieved_atoms_in_layer = achieved_atoms[idx];


    bool has_novel_atom = false;
    for (const Relation &relation : state.get_relations()) {
        int pred_symbol_idx = relation.predicate_symbol;
        for (const GroundAtom& tuple : relation.tuples) {
            auto it = atom_mapping[pred_symbol_idx].insert({tuple, atom_counter + 1});
            if (it.second) atom_counter++;
            bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k1(pred_symbol_idx, it.first->second);
            if (is_new)
                has_novel_atom = true;
        }
    }

    const vector<bool>& nullary_atoms = state.get_nullary_atoms();
    for (size_t i = 0; i < nullary_atoms.size(); ++i) {
        if (nullary_atoms[i]) {
            auto it = atom_mapping[i].insert({GroundAtom(), atom_counter + 1});
            if (it.second) atom_counter++;
            bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k1(i, it.first->second);
            if (is_new)
                has_novel_atom = true;
        }
    }

    if (has_novel_atom)
        return 1;
    else
        return NOVELTY_GREATER_THAN_TWO;

}


int StandardNovelty::compute_novelty_k2(const Task &task,
                                        const DBState &state,
                                        int number_unsatisfied_goals) {
    // Number of unsatisfied goals is computed as a side effect of compute_novelty_k1
    int novelty = compute_novelty_k1(task, state, number_unsatisfied_goals);
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
            int t1_idx = atom_mapping[pred_symbol_idx1][t1];
            for (const Relation &r2 : state.get_relations()) {
                int pred_symbol_idx2 = r2.predicate_symbol;
                if (pred_symbol_idx2 > pred_symbol_idx1) continue;
                for (const GroundAtom &t2 : r2.tuples) {
                    int t2_idx = atom_mapping[pred_symbol_idx2][t2];
                    bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(
                        compute_position(pred_symbol_idx1, pred_symbol_idx2),
                        t1_idx, t2_idx);
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
            int t1_idx = atom_mapping[pred_symbol_idx1][GroundAtom()];
            for (const Relation &r2 : state.get_relations()) {
                int pred_symbol_idx2 = r2.predicate_symbol;
                if (pred_symbol_idx2 > pred_symbol_idx1) continue;
                for (const GroundAtom &t2 : r2.tuples) {
                    int t2_idx = atom_mapping[pred_symbol_idx2][t2];
                    bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(
                                compute_position(pred_symbol_idx1, pred_symbol_idx2),
                                t1_idx, t2_idx);
                    if (is_new and !has_k1_novelty)
                        novelty = 2;
                }
            }
        }
    }

    return novelty;
}

int StandardNovelty::compute_position(int idx_1, int idx_2) {
    int row_start = (idx_1 * (idx_1+1))/2;
    int column_shift = idx_2;
    return row_start + column_shift;
}
