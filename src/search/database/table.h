#ifndef SEARCH_TABLE_H
#define SEARCH_TABLE_H

#include <boost/container/small_vector.hpp>

#include <vector>

/**
 * @brief Data-structure containing a set of tuples and the indices corresponding to
 * the free variable index of each tuple position.
 */
class Table {
public:
    // Same small-buffer-optimized type as GroundAtom (structures.h) so tuples
    // parsed from the state move into a Table without an element-wise copy. The
    // inline storage removes a heap allocation per join-output tuple on the hot
    // successor-generation path.
    using tuple_t = boost::container::small_vector<int, 4>;

    /// @var tuples: the relation corresponding to the table, encoded as a vector of tuples
    std::vector<tuple_t> tuples;
    /// @var tuple_index: Indices of each variable in order
    std::vector<int> tuple_index;

    Table(std::vector<tuple_t> &&tuples, std::vector<int> &&tuple_index) :
        tuples(std::move(tuples)),
        tuple_index(std::move(tuple_index))
    {}

    bool index_is_variable(std::size_t i) const {
        return tuple_index[i] >= 0;
    }

    Table() = default;

    static const Table& EMPTY_TABLE();
};


#endif //SEARCH_TABLE_H
