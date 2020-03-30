#ifndef SEARCH_GOALCOUNT_H
#define SEARCH_GOALCOUNT_H

#include "heuristic.h"

#include "../state.h"
#include "../task.h"

/**
 * @brief Compute hamming distance between goal condition and state s.
 *
 * @note Goal-aware. Inadmissible.
 *
 */
class Goalcount : public Heuristic {
public:
    int compute_heuristic(const State& s, const Task& task) final;

private:
  static int compute_reached_nullary_atoms(
      const std::unordered_set<int> &indices,
      const std::vector<bool> &nullary_atoms);
  int atom_not_satisfied(const State &s, const AtomicGoal &atomicGoal) const;
};

#endif //SEARCH_GOALCOUNT_H
