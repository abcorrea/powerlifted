#ifndef SEARCH_NOVELTY_STANDARD_NOVELTY_H_
#define SEARCH_NOVELTY_STANDARD_NOVELTY_H_

#include <utility>
#include <boost/functional/hash.hpp>

#include "../task.h"

#include "../heuristics/goalcount.h"
#include "../states/state.h"
#include "../structures.h"

typedef std::unordered_set<GroundAtom, boost::hash<std::vector<int>>> NoveltySet;

class AchievedGroundAtoms {

    std::vector<NoveltySet> ground_atoms_k1;

public:

    AchievedGroundAtoms() = default;

    AchievedGroundAtoms(const Task &task) :
        ground_atoms_k1(task.initial_state.get_relations().size()) {
    }

    bool try_to_insert_atom(int i, const GroundAtom& ga) {
        auto it = ground_atoms_k1[i].insert(ga);
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
