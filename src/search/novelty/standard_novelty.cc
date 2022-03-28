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


    bool has_n_ary_novelty, has_nullary_novelty;

    has_n_ary_novelty = compute_k1_novelty_of_n_ary_atoms(state, achieved_atoms_in_layer);
    has_nullary_novelty = compute_k1_novelty_of_nullary_atoms(state, achieved_atoms_in_layer);

    if (has_n_ary_novelty or has_nullary_novelty)
        return 1;
    else
        return NOVELTY_GREATER_THAN_TWO;

}

bool StandardNovelty::compute_k1_novelty_of_nullary_atoms(const DBState &state,
                                                          AchievedGroundAtoms &achieved_atoms_in_layer) {
    bool has_novel_atom = false;
    const vector<bool>& nullary_atoms = state.get_nullary_atoms();
    for (size_t i = 0; i < nullary_atoms.size(); ++i) {
        if (nullary_atoms[i]) {
            auto it = atom_mapping[i].insert({GroundAtom(), atom_counter + 1});
            if (it.second) atom_counter++;
            bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k1(i, it.first->second);
            if (is_new) {
                has_novel_atom = true;
            }
        }
    }
    return has_novel_atom;
}

bool StandardNovelty::compute_k1_novelty_of_n_ary_atoms(const DBState &state,
                                                        AchievedGroundAtoms &achieved_atoms_in_layer) {
    bool has_novel_atom = false;
    for (const Relation &relation : state.get_relations()) {
        int pred_symbol_idx = relation.predicate_symbol;
        for (const GroundAtom& tuple : relation.tuples) {
            auto it = atom_mapping[pred_symbol_idx].insert({tuple, atom_counter + 1});
            if (it.second) atom_counter++;
            bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k1(pred_symbol_idx, it.first->second);
            if (is_new) {
                has_novel_atom = true;
            }
        }
    }
    return has_novel_atom;
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

    int idx = compute_position_of_r_g_tuple(number_unsatisfied_goals, number_unsatisfied_relevant_atoms);

    bool has_k1_novelty = (novelty == 1);

    auto &achieved_atoms_in_layer = achieved_atoms[idx];

    novelty = std::min(novelty,
                       std::min(compute_k2_novelty_of_n_ary_atoms(state, has_k1_novelty, achieved_atoms_in_layer),
                                compute_k2_novelty_of_nullary_atoms(state, has_k1_novelty, achieved_atoms_in_layer)));
    return novelty;
}

int StandardNovelty::compute_k2_novelty_of_nullary_atoms(const DBState &state,
                                                         bool has_k1_novelty,
                                                         AchievedGroundAtoms &achieved_atoms_in_layer) {
    int novelty = NOVELTY_GREATER_THAN_TWO;
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
                    // We do not have the check if pred_symbol_idx2 == pred_symbol_idx1 because we always
                    // use the same empty tuple GroundAtom() for the nullary atoms.
                    bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(
                        compute_position_of_predicate_indices_pair(pred_symbol_idx1,
                                                                   pred_symbol_idx2),
                        t1_idx, t2_idx);
                    if (is_new and !has_k1_novelty) {
                        novelty = 2;
                    }
                }
            }
            for (size_t j = 0; j < nullary_atoms.size(); ++j) {
                if (nullary_atoms[j]) {
                    int pred_symbol_idx2 = j;
                    if (pred_symbol_idx2 > pred_symbol_idx1) continue;
                    // See comment above about the pred_symbol_idx2 == pred_symbol_idx1 check.
                    int t2_idx = atom_mapping[pred_symbol_idx2][GroundAtom()];
                    bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(
                        compute_position_of_predicate_indices_pair(pred_symbol_idx1,
                                                                   pred_symbol_idx2),
                        t1_idx, t2_idx);
                    if (is_new and !has_k1_novelty) {
                        novelty = 2;
                    }
                }
            }
        }
    }
    return novelty;
}

int StandardNovelty::compute_k2_novelty_of_n_ary_atoms(const DBState &state,
                                                       bool has_k1_novelty,
                                                       AchievedGroundAtoms &achieved_atoms_in_layer) {
    int novelty = NOVELTY_GREATER_THAN_TWO;
    for (const Relation &r1 : state.get_relations()) {
        int pred_symbol_idx1 = r1.predicate_symbol;
        for (const GroundAtom &t1 : r1.tuples) {
            int t1_idx = atom_mapping[pred_symbol_idx1][t1];
            for (const Relation &r2 : state.get_relations()) {
                int pred_symbol_idx2 = r2.predicate_symbol;
                // We do this check so we do not insert each atom 2x in the set.
                if (pred_symbol_idx2 > pred_symbol_idx1) continue;
                for (const GroundAtom &t2 : r2.tuples) {
                    int t2_idx = atom_mapping[pred_symbol_idx2][t2];
                    bool is_new = false;
                    // This case split exists so we always check things in a given cannonical order.
                    // If pred_symbol_idx2 > pred_symbol_id1, the check in the lines above already
                    // takes care of it.
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

    novelty = std::min(novelty,
                       compute_k1_novelty_from_operators(added_atoms, achieved_atoms_in_layer));

    if (width == 1) return novelty;

    bool has_k1_novelty = (novelty == 1);

    novelty = std::min(novelty,
                       compute_k2_novelty_from_operators(state,
                                                         added_atoms,
                                                         achieved_atoms_in_layer,
                                                         nullary_atoms,
                                                         has_k1_novelty));

    return novelty;
}

int StandardNovelty::compute_k2_novelty_from_operators(const DBState &state,
                                                       const vector<std::pair<int, GroundAtom>> &added_atoms,
                                                       AchievedGroundAtoms &achieved_atoms_in_layer,
                                                       const vector<bool> &nullary_atoms,
                                                       bool has_k1_novelty) {
    int novelty = NOVELTY_GREATER_THAN_TWO;
    for (const pair<int, GroundAtom> &r1 : added_atoms) {
        int pred_symbol_idx1 = r1.first;
        const GroundAtom &t1 = r1.second;
        int t1_idx = atom_mapping[pred_symbol_idx1][t1];
        for (const Relation &r2 : state.get_relations()) {
            int pred_symbol_idx2 = r2.predicate_symbol;

            // We cannot skip if pred_symbol_idx2 > pred_symbol_idx1, because added_atoms only has a subset
            // of the atoms. So, for example if added_atoms = {P(a)} and idx[P] = 1; we would never compare it
            // to atoms with predicate index of 0 if we skipped it -- because originally this comparison would
            // be done while looping through relation with idx 0. (See implementation of the original
            // functions without the optimization).

            for (const GroundAtom &t2 : r2.tuples) {
                int t2_idx = atom_mapping[pred_symbol_idx2][t2];
                bool is_new = check_tuple_novelty(achieved_atoms_in_layer,
                                                  pred_symbol_idx1,
                                                  t1_idx,
                                                  pred_symbol_idx2,
                                                  t2_idx);

                if (is_new and !has_k1_novelty) {
                    novelty = 2;
                }
            }
        }
        for (size_t i = 0; i < state.get_nullary_atoms().size(); ++i) {
            if (nullary_atoms[i]) {
                int pred_symbol_idx2 = i;
                // See above why we cannot skip the case where pred_symbol_idx2 > pred_symbol_idx1.
                int t2_idx = atom_mapping[pred_symbol_idx2][GroundAtom()];
                bool is_new = check_tuple_novelty(achieved_atoms_in_layer,
                                                  pred_symbol_idx1,
                                                  t1_idx,
                                                  pred_symbol_idx2,
                                                  t2_idx);
                if (is_new and !has_k1_novelty) {
                    novelty = 2;
                }
            }
        }
    }
    return novelty;
}


int StandardNovelty::compute_k1_novelty_from_operators(const vector<std::pair<int,
                                                                              GroundAtom>> &added_atoms,
                                                       AchievedGroundAtoms &achieved_atoms_in_layer) {
    int novelty = NOVELTY_GREATER_THAN_TWO;
    for (const pair<int, GroundAtom> &r1 : added_atoms) {
        int pred_symbol_idx = r1.first;
        const GroundAtom &tuple = r1.second;
        auto it = atom_mapping[pred_symbol_idx].insert({tuple, atom_counter + 1});
        if (it.second) atom_counter++;
        bool is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k1(pred_symbol_idx, it.first->second);
        if (is_new) {
            novelty = 1;
        }
    }
    return novelty;
}

bool StandardNovelty::check_tuple_novelty(AchievedGroundAtoms &achieved_atoms_in_layer,
                                          int pred_symbol_idx1,
                                          int t1_idx,
                                          int pred_symbol_idx2,
                                          int t2_idx) {
    // We have this split cases here, because in the case where we check all pair of atoms
    // in the state, we only compare cases where pred_symbol_idx2 > pred_symbol_idx1. So,
    // to keep the same "efficiency", we split the insertion in two cases below.
    // In the case where pred_symbol_idx2 = pred_symbol_idx1, we insert the tuples
    // in a canonical order.
    bool is_new;
    if (pred_symbol_idx2 < pred_symbol_idx1) {
        is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(
            compute_position_of_predicate_indices_pair(pred_symbol_idx1,
                                                       pred_symbol_idx2),
            t1_idx, t2_idx);
    } else if (pred_symbol_idx1 == pred_symbol_idx2) {
        is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(
            compute_position_of_predicate_indices_pair(pred_symbol_idx1,
                                                       pred_symbol_idx2),
            min(t1_idx, t2_idx), max(t1_idx, t2_idx));
    }
    else {
        // Note: tuples and predicate symbols are swapped here compared to the first case.
        is_new = achieved_atoms_in_layer.try_to_insert_atom_in_k2(
            compute_position_of_predicate_indices_pair(pred_symbol_idx2,
                                                       pred_symbol_idx1),
            t2_idx, t1_idx);
    }
    return is_new;
}
