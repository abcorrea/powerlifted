#ifndef SEARCH_STATE_H
#define SEARCH_STATE_H

#include "../structures.h"

#include <algorithm>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/functional/hash.hpp>

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

  friend std::size_t hash_value(const State &s) {
    std::size_t seed = 0;
    for (bool b : s.nullary_atoms) {
      boost::hash_combine(seed, b);
    }
    for (const Relation &r : s.relations) {
      std::vector<std::size_t> x;
      for (const GroundAtom &vga : r.tuples) {
        std::size_t aux_seed = vga.size();
        for (auto &i : vga)
          aux_seed ^= i + 0x9e3779b9 + (aux_seed << 6) + (aux_seed >> 2);
        x.push_back(aux_seed);
        //boost::hash_combine(seed, vga);
      }
      std::sort(x.begin(), x.end());
      for (std::size_t e : x) {
        boost::hash_combine(seed, e);
      }
    }
    return seed;
  }
};

/**
 * @brief Syntatic sugar to avoid passing the static predicated to
 * every single successor.
 */
typedef State StaticInformation;

#endif //SEARCH_STATE_H
