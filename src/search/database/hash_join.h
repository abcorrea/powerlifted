#ifndef SEARCH_HASH_JOIN_H
#define SEARCH_HASH_JOIN_H

#include <boost/functional/hash.hpp>

#include "table.h"

#include "../hash_structures.h"

void hash_join(Table &t1, Table &t2);

#endif //SEARCH_HASH_JOIN_H
