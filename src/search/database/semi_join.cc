
#include "semi_join.h"
#include "table.h"
#include "utils.h"

#include <algorithm>

using namespace std;

inline bool check_tuples_match(
    const vector<int> &tup1,
    const vector<int> &tup2,
    const std::vector<std::pair<int, int>>& columns_to_match)
{
    for (const auto& m:columns_to_match) {
        if (tup1[m.first] != tup2[m.second]) return false;
    }
    return true;
}

/**
* Semi-join two tables into one. t1 is the working table and will be modified.
*
* @details We first loop over the parameters of each table and check which indices match.
* Then, we split it into two cases:
* 1. If there are no matching indices, then we simply return
* 2. If at least one parameter matches, we perform a nested loop semi-join.
*
* We return the size of the semi-join in order to be able to identify when
* an empty relation is produced.
*
*/
std::size_t semi_join(CompactTable& t1, const CompactTable& t2) {

    auto matches = compute_matching_columns(t1, t2);

    if (matches.empty()) { // If no attribute matches, then we return
        return t1.num_valid();
    }

    // Otherwise, we perform the join and the projection
    auto sz1 = t1.size();
    auto sz2 = t2.size();

    for (unsigned i = 0; i < sz1; ++i) {
        if (!t1.is_valid(i)) continue;

        bool has_match = false;
        for (unsigned j = 0; j < sz2; ++j) {
            if (!t2.is_valid(j)) continue;
            if (check_tuples_match(t1.tuples()[i], t2.tuples()[j], matches)) {
                // If a matching tuple was found, no need to iterate further
                has_match = true;
                break;
            }
        }

        if (!has_match) {
            t1.invalidate(i);  // tuple at index i1 has no support
        }
    }

    return t1.num_valid();
}




