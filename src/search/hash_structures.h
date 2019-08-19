#ifndef SEARCH_HASH_STRUCTURES_H
#define SEARCH_HASH_STRUCTURES_H

#include <vector>

#include <boost/functional/hash/hash.hpp>

/**
 * @brief Generic hash function for vector of integers. The name "TupleHash" is used because its
 * main function is to hash tuples in relations/tables.
 *
 * @note Uses boost hash_range function
 */
struct TupleHash {
    std::size_t operator() (const std::vector<int> &c) const {
        return boost::hash_range(c.begin(), c.end());
    }
};


#endif //SEARCH_HASH_STRUCTURES_H
