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

typedef absl::flat_hash_map<GroundAtom, int, boost::hash<std::vector<int>>> NoveltySet;

class AchievedGroundAtoms {

    // TODO Check small vector optimizations.
    /* TODO Check also ArrayPool class (Scorpion).
     * Use ArrayPool to store vectors and each entry could have only the index to it.
     */

    int atom_counter;

    std::vector<NoveltySet> ground_atoms_k1;
    std::vector<std::vector<absl::flat_hash_set<std::pair<int, int>, boost::hash<std::pair<int, int>>>>> ground_atoms_k2;

public:

    AchievedGroundAtoms() = default;

    AchievedGroundAtoms(const Task &task) :
    atom_counter(0),
    ground_atoms_k1(task.initial_state.get_relations().size()) {
        // TODO Initialize only if width=2
        ground_atoms_k2.resize(task.initial_state.get_relations().size());
        for (auto &k : ground_atoms_k2) {
            k.resize(task.initial_state.get_relations().size());
        }
    }

    bool try_to_insert_atom_in_k1(int i, const GroundAtom& ga) {
        int next = atom_counter + 1;
        auto it = ground_atoms_k1[i].insert({ga, next});
        if (it.second) atom_counter++;
        return it.second;
    }

    bool try_to_insert_atom_in_k2(int i, int j, const GroundAtom& ga1, const GroundAtom& ga2) {
        int idx_ga1 = ground_atoms_k1[i][ga1];
        int idx_ga2 = ground_atoms_k1[j][ga2];
        int max = std::max(i, j);
        int min = std::min(i, j);
        auto it = ground_atoms_k2[max][min].insert({idx_ga1, idx_ga2});
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
    Goalcount gc;
    std::vector<AchievedGroundAtoms> achieved_atoms;

    int compute_unsatisfied_goals(const Task &task, const DBState &state);

public:

    static const int GOAL_STATE = 0;
    static const int NOVELTY_GREATER_THAN_TWO = 3;

    StandardNovelty(const Task &task) {
        achieved_atoms.resize(task.goal.positive_nullary_goals.size()
        + task.goal.negative_nullary_goals.size()
        + task.goal.goal.size(),
        AchievedGroundAtoms(task));
    }

    int compute_novelty_k1(const Task &task, const DBState &state);

    int compute_novelty_k2(const Task &task, const DBState &state);

};

#endif //SEARCH_NOVELTY_STANDARD_NOVELTY_H_
