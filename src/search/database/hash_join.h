#ifndef SEARCH_HASH_JOIN_H
#define SEARCH_HASH_JOIN_H

#include <boost/functional/hash.hpp>

#include "table.h"

struct TupleHash {
    std::size_t operator() (const std::vector<int> &c) const {
        return boost::hash_range(c.begin(), c.end());
    }
};

void hash_join(Table &t1, Table &t2);

#endif //SEARCH_HASH_JOIN_H
