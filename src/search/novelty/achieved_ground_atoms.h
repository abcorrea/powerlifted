#ifndef SEARCH_NOVELTY_ACHIEVED_GROUND_ATOMS_H_
#define SEARCH_NOVELTY_ACHIEVED_GROUND_ATOMS_H_

#include "../task.h"

#include "../parallel_hashmap/phmap.h"
#include "../parallel_hashmap/phmap_utils.h"

class AchievedGroundAtoms {

    // TODO Check small vector optimizations.
    /* TODO Check also ArrayPool class (Scorpion).
     * Use ArrayPool to store vectors and each entry could have only the index to it.
     */
    std::vector<phmap::flat_hash_set<int>> ground_atoms_k1;

    std::vector<phmap::flat_hash_set<std::pair<int, int>>> ground_atoms_k2;

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


#endif //SEARCH_NOVELTY_ACHIEVED_GROUND_ATOMS_H_
