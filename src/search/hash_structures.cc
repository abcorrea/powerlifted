
#include "hash_structures.h"

#include <boost/functional/hash/hash.hpp>

std::size_t TupleHash::operator()(const std::vector<int> &c) const {
    return boost::hash_range(c.begin(), c.end());
}
