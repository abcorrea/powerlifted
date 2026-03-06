
#include "hash_structures.h"

#include "utils/hash.h"

std::size_t TupleHash::operator()(const std::vector<int> &c) const
{
    return utils::hash_range(c.begin(), c.end());
}
