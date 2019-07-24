#ifndef SEARCH_SEMI_JOIN_H
#define SEARCH_SEMI_JOIN_H

#include <utility>
#include <vector>

#include "table.h"

#include "../hash_structures.h"

void semi_join(Table &t1, Table &t2);

#endif //SEARCH_SEMI_JOIN_H
