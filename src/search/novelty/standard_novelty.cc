#include "standard_novelty.h"

using namespace std;

int StandardNovelty::compute_novelty_k1(const Task &task,
                                        const DBState &state,
                                        int number_unsatisfied_goals,
                                        int number_unsatisfied_relevant_atoms) {
    if (number_unsatisfied_goals == 0) {
        return GOAL_STATE;
    }
    int idx = compute_position_of_r_g_tuple(number_unsatisfied_goals, number_unsatisfied_relevant_atoms);

    auto &achieved_atoms_in_layer = achieved_atoms[idx];


    bool has_novel_atom = false;
    for (const Relation &relation : state.get_relations()) {
        int pred_symbol_idx = relation.predicate_symbol;
        for (const GroundAtom& tuple : relation.tuples) {
            auto it = atom_mapping[pred_symbol_idx].insert({tuple, atom_counter + 1});
            if (it.second) atom_counter++;
            bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k1(pred_symbol_idx, it.first->second);
            if (is_new) {
                has_novel_atom = true;
                if (early_stop)
                    return has_novel_atom;
            }
        }
    }

    const vector<bool>& nullary_atoms = state.get_nullary_atoms();
    for (size_t i = 0; i < nullary_atoms.size(); ++i) {
        if (nullary_atoms[i]) {
            auto it = atom_mapping[i].insert({GroundAtom(), atom_counter + 1});
            if (it.second) atom_counter++;
            bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k1(i, it.first->second);
            if (is_new) {
                has_novel_atom = true;
                if (early_stop) return has_novel_atom;
            }
        }
    }

    if (has_novel_atom)
        return 1;
    else
        return NOVELTY_GREATER_THAN_TWO;

}


int StandardNovelty::compute_novelty(const Task &task,
                                     const DBState &state,
                                     int number_unsatisfied_goals,
                                     int number_unsatisfied_relevant_atoms) {
    // Number of unsatisfied goals is computed as a side effect of compute_novelty_k1
    int novelty = compute_novelty_k1(task, state, number_unsatisfied_goals, number_unsatisfied_relevant_atoms);

    if (novelty == 0) {
        // Goal state
        return 0;
    }

    if (width == 1) return novelty;

    if (early_stop and novelty == 1) {
        // If we have early stop flag activated and we found a novel atom, we return directly.
        return novelty;
    }

    int idx = compute_position_of_r_g_tuple(number_unsatisfied_goals, number_unsatisfied_relevant_atoms);

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
                    bool is_new = false;
                    if (pred_symbol_idx1 == pred_symbol_idx2) {
                        is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(
                            compute_position_of_predicate_indices_pair(pred_symbol_idx1,
                                                                       pred_symbol_idx2),
                            std::min(t1_idx, t2_idx), std::max(t1_idx, t2_idx));
                    }
                    else {
                        is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(
                            compute_position_of_predicate_indices_pair(pred_symbol_idx1,
                                                                       pred_symbol_idx2),
                            t1_idx, t2_idx);
                    }
                    if (is_new and !has_k1_novelty) {
                        novelty = 2;
                        if (early_stop) return novelty;
                    }
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
                        compute_position_of_predicate_indices_pair(pred_symbol_idx1,
                                                                   pred_symbol_idx2),
                        t1_idx, t2_idx);
                    if (is_new and !has_k1_novelty) {
                        novelty = 2;
                        if (early_stop)
                            return novelty;
                    }
                }
            }
            for (size_t j = i+1; j < nullary_atoms.size(); ++j) {
                if (nullary_atoms[j]) {
                    int pred_symbol_idx2 = j;
                    int t2_idx = atom_mapping[pred_symbol_idx2][GroundAtom()];
                    bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(
                        compute_position_of_predicate_indices_pair(pred_symbol_idx1,
                                                                   pred_symbol_idx2),
                        t1_idx, t2_idx);
                    if (is_new and !has_k1_novelty) {
                        novelty = 2;
                        if (early_stop) return novelty;
                    }
                }
            }
        }
    }

    return novelty;
}

int StandardNovelty::compute_position_of_predicate_indices_pair(int idx_1, int idx_2) {
    int row_start = (idx_1 * (idx_1+1))/2;
    int column_shift = idx_2;
    return row_start + column_shift;
}

int StandardNovelty::compute_novelty_from_operator(const Task &task,
                                                   const DBState &state,
                                                   int number_unsatisfied_goals,
                                                   int number_unsatisfied_relevant_atoms,
                                                   const std::vector<std::pair<int, GroundAtom>> & added_atoms) {

    if (number_unsatisfied_goals == 0) {
        return GOAL_STATE;
    }

    int novelty = NOVELTY_GREATER_THAN_TWO;

    int idx = compute_position_of_r_g_tuple(number_unsatisfied_goals, number_unsatisfied_relevant_atoms);
    auto &achieved_atoms_in_layer = achieved_atoms[idx];
    const vector<bool>& nullary_atoms = state.get_nullary_atoms();

    for (const std::pair<int, GroundAtom> &r1 : added_atoms) {
        int pred_symbol_idx = r1.first;
        const GroundAtom &tuple = r1.second;
        auto it = atom_mapping[pred_symbol_idx].insert({tuple, atom_counter + 1});
        if (it.second) atom_counter++;
        bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k1(pred_symbol_idx, it.first->second);
        if (is_new) {
            novelty = 1;
            if (early_stop) return novelty;
        }
    }


    if (width == 1) return novelty;

    if (early_stop and novelty == 1) return novelty;

    bool has_k1_novelty = (novelty == 1);

    for (const std::pair<int, GroundAtom> &r1 : added_atoms) {
        int pred_symbol_idx1 = r1.first;
        const GroundAtom &t1 = r1.second;
        int t1_idx = atom_mapping[pred_symbol_idx1][t1];
        for (const Relation &r2 : state.get_relations()) {
            int pred_symbol_idx2 = r2.predicate_symbol;

            for (const GroundAtom &t2 : r2.tuples) {
                int t2_idx = atom_mapping[pred_symbol_idx2][t2];
                bool is_new = false;

                // We have this split case here, because in the case where we check all pair of atoms
                // in the state, we only compare cases where pred_symbol_idx2 > pred_symbol_idx1. So,
                // to keep the same "efficiency", we split the insertion in two cases below.
                if (pred_symbol_idx2 < pred_symbol_idx1) {
                    is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(
                        compute_position_of_predicate_indices_pair(pred_symbol_idx1,
                                                                   pred_symbol_idx2),
                        t1_idx, t2_idx);
                } else if (pred_symbol_idx1 == pred_symbol_idx2) {
                    is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(
                        compute_position_of_predicate_indices_pair(pred_symbol_idx1,
                                                                   pred_symbol_idx2),
                        std::min(t1_idx, t2_idx), std::max(t1_idx, t2_idx));
                }
                else {
                    is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(
                        compute_position_of_predicate_indices_pair(pred_symbol_idx2,
                                                                   pred_symbol_idx1),
                        t2_idx, t1_idx);
                }

                if (is_new and !has_k1_novelty) {
                    novelty = 2;
                    if (early_stop) return novelty;
                }
            }
        }
        for (size_t i = 0; i < state.get_nullary_atoms().size(); ++i) {
            if (nullary_atoms[i]) {
                int pred_symbol_idx2 = i;
                int t2_idx = atom_mapping[pred_symbol_idx2][GroundAtom()];
                bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(
                    compute_position_of_predicate_indices_pair(pred_symbol_idx1,
                                                               pred_symbol_idx2),
                    t1_idx, t2_idx);
                if (is_new and !has_k1_novelty) {
                    novelty = 2;
                    if (early_stop) return novelty;
                }
            }
        }
    }


    return novelty;
}
