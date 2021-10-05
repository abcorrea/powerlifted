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

    AchievedGroundAtoms(const Task &task) {
        nullary_atoms.resize(task.initial_state.get_nullary_atoms().size(), false);
    }

    bool empty() {
        return ground_atoms.empty();
    }

    void insert(int i, const GroundAtom& ga) {
        ground_atoms.emplace(i, ga);
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
        + task.goal.goal.size());

        compute_novelty(task, task.initial_state);

    }

    int compute_novelty(const Task &task, const DBState &state) {
        int number_unsatisfied_goals = gc.compute_heuristic(state, task);
        if (number_unsatisfied_goals == 0) {
            return GOAL;
        }
        int idx = number_unsatisfied_goals - 1;

        auto &achieved_atoms_in_layer = achieved_atoms[idx];

        bool has_novel_atom = false;
        for (const Relation &relation : state.get_relations()) {
            int pred_symbol_idx = relation.predicate_symbol;
            for (const GroundAtom& tuple : relation.tuples) {
                bool is_new = achieved_atoms_in_layer.try_to_insert(pred_symbol_idx,
                                                                tuple);
                if (is_new)
                    has_novel_atom = true;
            }
        }

        const std::vector<bool>& nullary_atoms = state.get_nullary_atoms();
        for (size_t i = 0; i < nullary_atoms.size(); ++i) {
            if (nullary_atoms[i]) {
                bool is_new = achieved_atoms_in_layer.try_to_insert_nullary_atom(i);
                if (is_new)
                    has_novel_atom = true;
            }
        }

        if (has_novel_atom)
            return NOVEL;
        else
            return NOT_NOVEL;

    }

};

#endif //SEARCH_NOVELTY_STANDARD_NOVELTY_H_
