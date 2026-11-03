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
 * The novelty of a state is the size of the smallest set of facts (up to the
 * width, at most 3) that it makes true for the first time in its partition,
 * or NOT_NOVEL. A table of width w needs num_facts^w bits, so tasks with
 * more than MAX_FACTS_FOR_WIDTH_2 (MAX_FACTS_FOR_WIDTH_3) facts use smaller
 * widths, and partitions beyond a memory limit get smaller tables.
 */
class GroundNovelty {
    struct Table {
        std::vector<uint64_t> singles;
        std::vector<uint64_t> pairs;
        std::vector<uint64_t> triples;
    };
    int width;
    int num_facts;
    int num_words;
    std::unordered_map<long long, Table> tables;
    size_t pair_table_bytes = 0;
    size_t triple_table_bytes = 0;

    //! Tuples (pairs and triples) considered by the checks, and number of
    //! checks.
    long long num_tuples_checked = 0;
    long long num_checks = 0;
    //! Average number of tuples per check beyond which the width is reduced.
    long long max_tuples_per_check = 100000;

    Table &get_table(long long partition);
    int get_novelty(const Table &table, bool novel_fact, bool novel_pair, bool novel_triple) const;

public:
    static const int NOT_NOVEL = 4;
    static const int MAX_FACTS_FOR_WIDTH_2 = 4096;
    static const int MAX_FACTS_FOR_WIDTH_3 = 800;

    GroundNovelty(int width, int num_facts);

    int get_width() const { return width; }

    /**
     * Novelty of the state with the given bits and true facts (in increasing
     * order; only needed for width 2 and 3) in the given partition.
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
     * Whether the checks of pairs and triples cost too much per state (e.g.,
     * in tasks with many true facts per state, or long forced chains
     * changing many facts).
     */
    bool is_too_expensive() const;

    //! Reduce the width by one, freeing the tables of the larger tuples.
    void downgrade();

    //! Forget all states seen so far.
    void reset();

    void set_max_tuples_per_check(long long value) { max_tuples_per_check = value; }
};

#endif  // SEARCH_NOVELTY_GROUND_NOVELTY_H
