#ifndef SEARCH_NOVELTY_GROUND_NOVELTY_H
#define SEARCH_NOVELTY_GROUND_NOVELTY_H

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

/**
 * @brief Novelty of states of the ground task (bitsets over its facts), with
 * one novelty table per partition of the states.
 *
 * The novelty of a state is 1 if it makes some fact true for the first time
 * in its partition, 2 if it makes some pair of facts true for the first time
 * (width 2 only), and NOT_NOVEL otherwise. Width 2 needs a table of
 * num_facts^2 bits per partition, so tasks with more than
 * MAX_FACTS_FOR_WIDTH_2 facts use width 1, and partitions beyond a memory
 * limit get width-1 tables only.
 */
class GroundNovelty {
    struct Table {
        std::vector<uint64_t> singles;
        std::vector<uint64_t> pairs;
    };
    int width;
    int num_facts;
    int num_words;
    std::unordered_map<long long, Table> tables;
    size_t pair_table_bytes = 0;

    //! Pairs considered by the width-2 checks, and number of checks.
    long long num_pairs_checked = 0;
    long long num_checks = 0;

    Table &get_table(long long partition);

public:
    static const int NOT_NOVEL = 3;
    static const int MAX_FACTS_FOR_WIDTH_2 = 4096;

    GroundNovelty(int width, int num_facts);

    int get_width() const { return width; }

    /**
     * Novelty of the state with the given bits and true facts (in increasing
     * order; only needed for width 2) in the given partition.
     */
    int compute(const uint64_t *bits, const std::vector<int> &facts, long long partition);

    /**
     * Novelty of a state whose parent belongs to the same partition: only
     * the facts added with respect to the parent can be new.
     */
    int compute_for_added_facts(const std::vector<int> &added,
                                const std::vector<int> &facts,
                                long long partition);

    /**
     * Whether width-2 checks cost too much per state (e.g., in tasks with
     * many true facts per state, or long forced chains changing many facts).
     */
    bool is_too_expensive() const;

    //! Use width 1 from now on, freeing the pair tables.
    void downgrade_to_width_1();
};

#endif  // SEARCH_NOVELTY_GROUND_NOVELTY_H
