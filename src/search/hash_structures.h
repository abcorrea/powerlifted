#ifndef SEARCH_HASH_STRUCTURES_H
#define SEARCH_HASH_STRUCTURES_H

#include <vector>

#include <boost/functional/hash/hash.hpp>

struct TupleHash {
    std::size_t operator() (const std::vector<int> &c) const {
        return boost::hash_range(c.begin(), c.end());
    }
};


#endif //SEARCH_HASH_STRUCTURES_H
