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
    int number_relations;
    std::vector<absl::flat_hash_set<int>> ground_atoms_k1;

    // TODO For vector entry I we only need an entry for relations I <= J <= K.
    //  How can we extract indices directly from it?
    std::vector<absl::flat_hash_set<std::pair<int, int>>> ground_atoms_k2;


    size_t compute_index(int i, int j) const {
        return i*number_relations + j;
    }

public:

    AchievedGroundAtoms() = default;

    AchievedGroundAtoms(const Task &task) :
    ground_atoms_k1(task.initial_state.get_relations().size()) {
        number_relations = task.initial_state.get_relations().size();

        // TODO Initialize only if width=2
        ground_atoms_k2.resize(number_relations * number_relations);
    }

    bool try_to_insert_atom_in_k1(int i, int idx) {
        auto it = ground_atoms_k1[i].insert(idx);
        return it.second;
    }

    bool try_to_insert_atom_in_k2(int i, int j, int idx_ga1, int idx_ga2) {
        auto it = ground_atoms_k2[compute_index(i, j)].insert({idx_ga1, idx_ga2});
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

    int number_unsatisfied_goals;
    int atom_counter;
    Goalcount gc;
    std::vector<AchievedGroundAtoms> achieved_atoms;
    std::vector<NoveltySet> atom_mapping;

    int compute_unsatisfied_goals(const Task &task, const DBState &state);

public:

    static const int GOAL_STATE = 0;
    static const int NOVELTY_GREATER_THAN_TWO = 3;

    StandardNovelty(const Task &task) : atom_counter(0) {
        achieved_atoms.resize(task.goal.positive_nullary_goals.size()
        + task.goal.negative_nullary_goals.size()
        + task.goal.goal.size(),
        AchievedGroundAtoms(task));
        atom_mapping.resize(task.initial_state.get_relations().size());
    }

    int compute_novelty_k1(const Task &task, const DBState &state);

    int compute_novelty_k2(const Task &task, const DBState &state);

};

#endif //SEARCH_NOVELTY_STANDARD_NOVELTY_H_
