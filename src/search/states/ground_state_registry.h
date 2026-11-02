#ifndef SEARCH_STATES_GROUND_STATE_REGISTRY_H
#define SEARCH_STATES_GROUND_STATE_REGISTRY_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

/**
 * @brief Stores the states of the ground task (bitsets over its facts, see
 * states/ground_state.h) and assigns them consecutive ids, using an
 * open-addressing hash table of state ids.
 *
 * States are stored as bitsets in blocks of contiguous memory. In compact
 * mode (for tasks with many facts), each state is stored instead as
 * whichever is smaller: the sorted list of its true facts or its bitset (as
 * 32-bit halves). The choice depends only on the state, so equal states have
 * equal representations.
 */
class GroundStateRegistry {
    static const int BLOCK_BITS = 12;
    static const int BLOCK_SIZE = 1 << BLOCK_BITS;

    int num_words;
    int num_states;
    std::vector<uint32_t> state_hashes;
    std::vector<int> table;
    size_t mask;

    // Bitsets of the states (if not compact).
    std::vector<std::unique_ptr<uint64_t[]>> blocks;

    bool compact;
    // Compact mode: the representation of state i is
    // compact_data[compact_start[i] .. compact_start[i + 1]), a fact list if
    // is_list[i], otherwise the bitset words split into 32-bit halves.
    std::vector<uint32_t> compact_data;
    std::vector<size_t> compact_start;
    std::vector<char> is_list;
    std::vector<uint32_t> encoded;

    uint64_t *get_bits(int id);
    const uint64_t *get_bits(int id) const;
    //! Compact representation of the state (in `encoded`); true if a list.
    bool encode(const uint64_t *bits);
    bool is_equal(int id, uint32_t hash, const uint64_t *bits, bool list) const;
    void grow();

public:
    explicit GroundStateRegistry(int num_facts);

    bool is_compact() const { return compact; }

    int size() const { return num_states; }

    //! Write the bitset of state `id` to `bits`.
    void copy_to(int id, uint64_t *bits) const;

    //! Insert the state (if new). Returns its id and whether it is new.
    std::pair<int, bool> insert(const uint64_t *bits);
};

#endif  // SEARCH_STATES_GROUND_STATE_REGISTRY_H
