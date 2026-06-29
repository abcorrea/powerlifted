#ifndef SEARCH_HASH_STRUCTURES_H
#define SEARCH_HASH_STRUCTURES_H

#include "utils/hash.h"

#include <vector>


/**
 * @brief Generic hash function for integer tuples (relations/tables). Templated
 * on the container so it works for both std::vector<int> (join keys) and the
 * small-buffer-optimized GroundAtom/tuple_t; it only depends on the contents,
 * so a small_vector and a std::vector with the same ints hash identically.
 *
 * @note Uses utils::hash_range function
 */
struct TupleHash {
    template <typename Tuple>
    std::size_t operator()(const Tuple &c) const
    {
        return utils::hash_range(c.begin(), c.end());
    }
};

#endif  // SEARCH_HASH_STRUCTURES_H
