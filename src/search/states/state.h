#ifndef SEARCH_STATE_H
#define SEARCH_STATE_H

#include "../structures.h"

#include <algorithm>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>


/**
 * @brief Represents a state in the search space. Intuitively, it is represented
 * as a list of relations (tables).
 *
 * @details A state has a vector of Relations and a boolean vector indicating
 * which nullary atoms are true in the given state. This representation is sometimes
 * called 'sparse', because we do not have a value for every (relaxed) reachable
 * ground atom of the task.
 * For motivation on the use of sparse state representation,
 * see A. B. Correa, 2019.'Planning using Lifted Task Representations',
 * M.Sc. thesis. University of Basel.
 *
 * @see state_packer.h
 *
 */
class State {

 public:
  std::vector<Relation> relations;
  std::vector<bool> nullary_atoms;

  State() = default;
  explicit State(unsigned num_predicates) :
      relations(num_predicates), nullary_atoms(num_predicates, false)
  {}

  State(std::vector<Relation> &&relations, std::vector<bool> &&nullary_atoms) :
    relations(std::move(relations)), nullary_atoms(std::move(nullary_atoms))
  {
    // Explicit state constructor
  }

  std::vector<int> getObjects();

  void addTuple(int relation, const GroundAtom &args);

  bool operator==(const State &other) const {
      return nullary_atoms == other.nullary_atoms && relations == other.relations;
  }

  friend std::size_t hash_value(const State &s);
};

/**
 * @brief Syntatic sugar to avoid passing the static predicated to
 * every single successor.
 */
typedef State StaticInformation;

#endif //SEARCH_STATE_H
