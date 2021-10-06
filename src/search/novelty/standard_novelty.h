#ifndef SEARCH_NOVELTY_STANDARD_NOVELTY_H_
#define SEARCH_NOVELTY_STANDARD_NOVELTY_H_

#include <utility>
#include <boost/functional/hash.hpp>

#include "../task.h"

#include "../heuristics/goalcount.h"
#include "../states/state.h"
#include "../structures.h"


class AchievedGroundAtoms {

    std::unordered_set<std::pair<int, GroundAtom>,
        boost::hash<std::pair<int, std::vector<int>>>> ground_atoms;
    std::vector<bool> nullary_atoms;

public:

    AchievedGroundAtoms() = default;

    AchievedGroundAtoms(const Task &task) :
        nullary_atoms(task.initial_state.get_nullary_atoms().size(), false) {
        std::cout << "SIZE: " << nullary_atoms.size() << std::endl;
    }

    bool try_to_insert(int i, const GroundAtom& ga) {
        auto it = ground_atoms.insert(make_pair(i, ga));
        return it.second;
    }

    bool try_to_insert_nullary_atom(size_t i) {
        if (nullary_atoms[i]) return false; // Atom was already achieved before
        nullary_atoms[i] = true;
        return true;
    }

};


/*
 *
 * This implements the evaluator R_0 from Frances et al (IJCAI-17) -- assuming it is combined
 * with the BFWS class.
 *
 */
class StandardNovelty {

    Goalcount gc;
    std::vector<AchievedGroundAtoms> achieved_atoms;

public:

    static const int GOAL = 0;
    static const int NOVEL = 1;
    static const int NOT_NOVEL = 2;

    StandardNovelty(const Task &task) {
        achieved_atoms.resize(task.goal.positive_nullary_goals.size()
        + task.goal.negative_nullary_goals.size()
        + task.goal.goal.size(),
        AchievedGroundAtoms(task));

        compute_novelty(task, task.initial_state);

    }

    int compute_novelty(const Task &task, const DBState &state);

};

#endif //SEARCH_NOVELTY_STANDARD_NOVELTY_H_
