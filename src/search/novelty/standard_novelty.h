#ifndef SEARCH_NOVELTY_STANDARD_NOVELTY_H_
#define SEARCH_NOVELTY_STANDARD_NOVELTY_H_

#include <utility>

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <boost/functional/hash.hpp>
#include <boost/container/small_vector.hpp>

#include "../task.h"

#include "../heuristics/goalcount.h"
#include "../states/state.h"
#include "../structures.h"

typedef absl::flat_hash_map<GroundAtom, int> NoveltySet;

class AchievedGroundAtoms {

    // TODO Check small vector optimizations.
    /* TODO Check also ArrayPool class (Scorpion).
     * Use ArrayPool to store vectors and each entry could have only the index to it.
     */
    std::vector<absl::flat_hash_set<int>> ground_atoms_k1;

    std::vector<absl::flat_hash_set<std::pair<int, int>>> ground_atoms_k2;

public:

    AchievedGroundAtoms() = default;

    AchievedGroundAtoms(const Task &task, size_t number_combinations) :
    ground_atoms_k1(task.initial_state.get_relations().size()) {
        // TODO Initialize only if width=2
        ground_atoms_k2.resize(number_combinations);
    }

    bool try_to_insert_atom_in_k1(int i, int idx) {
        auto it = ground_atoms_k1[i].insert(idx);
        return it.second;
    }

    bool try_to_insert_atom_in_k2(int idx, int idx_ga1, int idx_ga2) {
        auto it = ground_atoms_k2[idx].insert({idx_ga1, idx_ga2});
        return it.second;
    }

};


/*
 *
 * This implements the evaluator R_0 from Frances et al (IJCAI-17) -- assuming it is combined
 * with the BFWS class.
 *
 */
class StandardNovelty {

    int atom_counter;
    Goalcount gc;
    std::vector<AchievedGroundAtoms> achieved_atoms;
    std::vector<NoveltySet> atom_mapping;

public:

    static const int GOAL_STATE = 0;
    static const int NOVELTY_GREATER_THAN_TWO = 3;

    StandardNovelty(const Task &task) : atom_counter(0) {
        size_t n_relations = task.initial_state.get_relations().size();
        atom_mapping.resize(n_relations);
        int max_position = compute_position(n_relations-1, n_relations);
        achieved_atoms.resize(task.goal.positive_nullary_goals.size()
                                  + task.goal.negative_nullary_goals.size()
                                  + task.goal.goal.size(),
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
