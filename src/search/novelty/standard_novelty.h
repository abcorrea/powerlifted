#ifndef SEARCH_NOVELTY_STANDARD_NOVELTY_H_
#define SEARCH_NOVELTY_STANDARD_NOVELTY_H_

#include <utility>

#include <boost/functional/hash.hpp>
#include <boost/container/small_vector.hpp>

#include "achieved_ground_atoms.h"

#include "../task.h"

#include "../heuristics/goalcount.h"
#include "../states/state.h"
#include "../structures.h"
#include "../action.h"

#include "../parallel_hashmap/phmap.h"
#include "../utils/hash.h"

typedef phmap::flat_hash_map<GroundAtom, int, utils::Hash<GroundAtom>> NoveltySet;

/*
 *
 * This implements the evaluator R_0 from Frances et al (IJCAI-17) -- assuming it is combined
 * with the BFWS class.
 *
 */
class StandardNovelty {

    int atom_counter;
    int number_goal_atoms;
    int number_relevant_atoms;
    int width;
    std::vector<AchievedGroundAtoms> achieved_atoms;
    std::vector<NoveltySet> atom_mapping;

    int compute_position_of_predicate_indices_pair(int idx_1, int idx_2);

    int compute_position_of_r_g_tuple(int unreached_goal_atoms, int unreached_relevant_atoms) {
        /*
         * Compute for a vector <#g, #r> the position of the tuple in the vector
         */
        return unreached_relevant_atoms * (number_goal_atoms+1) + unreached_goal_atoms;
    }

    int compute_novelty_k1(const Task &task,
                           const DBState &state,
                           int number_unsatisfied_goals,
                           int number_unsatisfied_relevant_atoms);

    bool compute_k1_novelty_of_n_ary_atoms(const DBState &state,
                                           AchievedGroundAtoms &achieved_atoms_in_layer);
    bool compute_k1_novelty_of_nullary_atoms(const DBState &state,
                                             AchievedGroundAtoms &achieved_atoms_in_layer);
    int compute_k2_novelty_of_n_ary_atoms(const DBState &state,
                                          bool has_k1_novelty,
                                          AchievedGroundAtoms &achieved_atoms_in_layer);
    int compute_k2_novelty_of_nullary_atoms(const DBState &state,
                                            bool has_k1_novelty,
                                            AchievedGroundAtoms &achieved_atoms_in_layer);
    int compute_k1_novelty_from_operators(const std::vector<std::pair<int,
                                                                      GroundAtom>> &added_atoms,
                                          AchievedGroundAtoms &achieved_atoms_in_layer);
    int compute_k2_novelty_from_operators(const DBState &state,
                                          const std::vector<std::pair<int,
                                                                      GroundAtom>> &added_atoms,
                                          AchievedGroundAtoms &achieved_atoms_in_layer,
                                          const std::vector<bool> &nullary_atoms,
                                          bool has_k1_novelty);

    bool check_tuple_novelty(AchievedGroundAtoms &achieved_atoms_in_layer,
                             int pred_symbol_idx1,
                             int t1_idx,
                             int pred_symbol_idx2,
                             int t2_idx);


public:

    static const int GOAL_STATE = 0;
    static const int NOVELTY_GREATER_THAN_TWO = 3;

    static const int R_0 = 0;
    static const int R_X = 1;
    static const int IW = 2;
    static const int IW_G = 3;

    StandardNovelty(const Task &task,
                    size_t number_goal_atoms,
                    size_t number_relevant_atoms,
                    int width) : atom_counter(0),
                                                    number_goal_atoms(number_goal_atoms),
                                                    number_relevant_atoms(number_relevant_atoms),
                                                    width(width) {
        std::cout << "Total number of goal atoms: " << number_goal_atoms << std::endl;
        std:: cout << "Total number of relevant atoms: " << number_relevant_atoms << std::endl;

        size_t n_relations = task.initial_state.get_relations().size();

        atom_mapping.resize(n_relations);

        int max_position = compute_position_of_predicate_indices_pair(n_relations - 1, n_relations);
        achieved_atoms.resize((number_relevant_atoms+1) * (number_goal_atoms+1),
                              AchievedGroundAtoms(task, max_position));
    }

    int compute_novelty(const Task &task,
                        const DBState &state,
                        int number_unsatisfied_goals,
                        int number_unsatisfied_relevant_atoms);

    int compute_novelty_from_operator(const Task &task,
                                      const DBState &state,
                                      int number_unsatisfied_goals,
                                      int number_unsatisfied_relevant_atoms,
                                      const std::vector<std::pair<int, std::vector<int>>> &added_atoms);

    int get_number_relevant_atoms() const {
        return number_relevant_atoms;
    }

};

#endif //SEARCH_NOVELTY_STANDARD_NOVELTY_H_
