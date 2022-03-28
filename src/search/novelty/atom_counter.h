#ifndef SEARCH_NOVELTY_ATOM_COUNTER_H_
#define SEARCH_NOVELTY_ATOM_COUNTER_H_

#include "../structures.h"

/*
 * This class simply stores a set of atoms and, given a state, checks how many of these atoms
 * are true in the given state.
 *
 * More info: some local profiling showed that the atom counter (for BFWS with R[X]) accounts for
 * less than 3% of the total time.
 *
 */
class AtomCounter {

    std::vector<std::vector<GroundAtom>> atoms;
    std::unordered_set<int> positive_nullary;
    std::unordered_set<int> negative_nullary;


public:
    AtomCounter() = default;

    AtomCounter(const std::vector<std::vector<GroundAtom>> &atoms,
                const std::unordered_set<int> &positive,
                const std::unordered_set<int> &negative) :
        atoms(atoms), positive_nullary(positive), negative_nullary(negative) {}

    int compute_unreached_nullary_atoms(const std::vector<bool> &nullary_atoms) const {
        int h = 0;
        for (int pred : positive_nullary) {
            if (!nullary_atoms[pred]) {
                h++;
            }
        }
        for (int pred : negative_nullary) {
            if (nullary_atoms[pred]) {
                h++;
            }
        }
        return h;
    }

    int count_unachieved_atoms(const DBState &state, const Task &task) const {
        int count = 0;

        count += compute_unreached_nullary_atoms(state.get_nullary_atoms());

        for (size_t i = 0; i < atoms.size(); ++i) {
            for (const auto &tuple : atoms[i]) {
                if (state.get_tuples_of_relation(i).count(tuple) == 0)
                    ++count;
            }
        }
        return count;
    }

    size_t get_total_number_of_atoms() {
        size_t s = positive_nullary.size() + negative_nullary.size();
        for (const auto &v : atoms) {
            s += v.size();
        }
        return s;
    }
};

#endif //SEARCH_NOVELTY_ATOM_COUNTER_H_
