#ifndef SEARCH_NOVELTY_STANDARD_NOVELTY_H_
#define SEARCH_NOVELTY_STANDARD_NOVELTY_H_

#include <utility>

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <boost/functional/hash.hpp>
#include <boost/container/small_vector.hpp>

#include "achieved_ground_atoms.h"

#include "../task.h"

#include "../heuristics/goalcount.h"
#include "../states/state.h"
#include "../structures.h"

typedef absl::flat_hash_map<GroundAtom, int> NoveltySet;


/*
 *
 * This implements the evaluator R_0 from Frances et al (IJCAI-17) -- assuming it is combined
 * with the BFWS class.
 *
 */
class StandardNovelty {

    int atom_counter;
    std::vector<AchievedGroundAtoms> achieved_atoms;
    std::vector<NoveltySet> atom_mapping;

public:

    static const int GOAL_STATE = 0;
    static const int NOVELTY_GREATER_THAN_TWO = 3;

    StandardNovelty(const Task &task, size_t number_atoms) : atom_counter(0) {
        size_t n_relations = task.initial_state.get_relations().size();
        atom_mapping.resize(n_relations);
        int max_position = compute_position(n_relations-1, n_relations);
        achieved_atoms.resize(number_atoms,
                              AchievedGroundAtoms(task, max_position));
    }

    int compute_novelty_k1(const Task &task,
                           const DBState &state,
                           int number_unsatisfied_goals);

    int compute_novelty_k2(const Task &task,
                           const DBState &state,
                           int number_unsatisfied_goals);

    int compute_position(int idx_1, int idx_2);
};

#endif //SEARCH_NOVELTY_STANDARD_NOVELTY_H_
