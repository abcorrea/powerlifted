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

public:

    AchievedGroundAtoms() = default;

    bool empty() {
        return ground_atoms.empty();
    }

    void insert(int i, GroundAtom ga) {
        ground_atoms.emplace(i, ga);
    }

    bool try_to_insert(int i, GroundAtom ga) {
        auto it = ground_atoms.insert(make_pair(i, ga));
        return it.second;
    }

};

class StandardNovelty {

    Goalcount gc;
    std::vector<AchievedGroundAtoms> hashmap;



public:

    static const int NOVEL = 1;
    static const int NOT_NOVEL = 2;

    StandardNovelty(const Task &task) {
        hashmap.resize(task.goal.positive_nullary_goals.size()
        + task.goal.negative_nullary_goals.size()
        + task.goal.goal.size());

        compute_novelty(task, task.initial_state);

    }

    int compute_novelty(const Task &task, const DBState &state) {
        int number_unsatisfied_goals = gc.compute_heuristic(state, task);
        if (number_unsatisfied_goals == 0) {
            return NOVEL;
        }
        int idx = number_unsatisfied_goals - 1;

        if (idx > int(hashmap.size())) {
            std::cerr << "ERROR in computation of novelty value." << std::endl;
            exit(-1);
        }

        if (hashmap[idx].empty()) {
            size_t relation_counter = 0;
            for (const Relation &relation : state.get_relations()) {
                for (GroundAtom tuple : relation.tuples) {
                    GroundAtom t = tuple;
                    hashmap[idx].insert(relation_counter, t);
                }
                ++relation_counter;
            }
            for (bool null_atom : state.get_nullary_atoms()) {
                if (null_atom) {
                    GroundAtom empty_tuple;
                    hashmap[idx].insert(relation_counter, empty_tuple);
                }
                ++relation_counter;
            }
            return NOVEL;
        }

        int relation_counter = 0;
        //task.dump_state(state);
        for (const Relation &relation : state.get_relations()) {
            for (GroundAtom tuple : relation.tuples) {
                GroundAtom t = tuple;
                bool is_new = hashmap[idx].try_to_insert(relation_counter,
                                                         t);
                if (is_new)
                    return NOVEL;
            }
            relation_counter++;
        }
        for (bool null_atom : state.get_nullary_atoms()) {
                if (null_atom) {
                    GroundAtom empty_tuple;
                    bool is_new = hashmap[idx].try_to_insert(relation_counter, empty_tuple);

                    if (is_new)
                        return NOVEL;
                }
                ++relation_counter;
            }


        return NOT_NOVEL;

    }

};

#endif //SEARCH_NOVELTY_STANDARD_NOVELTY_H_
