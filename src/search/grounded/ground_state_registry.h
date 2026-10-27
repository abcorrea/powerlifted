#ifndef SEARCH_GROUNDED_GROUND_STATE_REGISTRY_H
#define SEARCH_GROUNDED_GROUND_STATE_REGISTRY_H

#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

/**
 * @brief Stores ground states as fixed-size bitsets (one bit per fact) in
 * blocks of contiguous memory, and indexes them with an open-addressing hash
 * table of state ids.
 */
class GroundStateRegistry {
    static const int BLOCK_BITS = 12;
    static const int BLOCK_SIZE = 1 << BLOCK_BITS;

    int num_words;
    int num_states;
    std::vector<std::unique_ptr<uint64_t[]>> blocks;
    std::vector<uint32_t> state_hashes;
    std::vector<int> table;
    size_t mask;

    static uint64_t hash_words(const uint64_t *words, int n) {
        uint64_t h = 0x9E3779B97F4A7C15ULL ^ uint64_t(n);
        for (int i = 0; i < n; ++i) {
            h ^= words[i] + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
        }
        h ^= h >> 33;
        h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 33;
        h *= 0xc4ceb9fe1a85ec53ULL;
        h ^= h >> 33;
        return h;
    }

    void grow() {
        size_t new_size = table.size() * 2;
        std::vector<int> new_table(new_size, -1);
        size_t new_mask = new_size - 1;
        for (int id : table) {
            if (id < 0)
                continue;
            size_t pos = state_hashes[id] & new_mask;
            while (new_table[pos] >= 0)
                pos = (pos + 1) & new_mask;
            new_table[pos] = id;
        }
        table.swap(new_table);
        mask = new_mask;
    }

public:
    explicit GroundStateRegistry(int num_facts)
        : num_words((num_facts + 63) / 64), num_states(0), table(1 << 16, -1),
          mask((1 << 16) - 1) {
        if (num_words == 0)
            num_words = 1;
    }

    int get_num_words() const {
        return num_words;
    }

    int size() const {
        return num_states;
    }

    const uint64_t *get(int id) const {
        return blocks[id >> BLOCK_BITS].get() + size_t(id & (BLOCK_SIZE - 1)) * num_words;
    }

    /**
     * Insert the state (if new). Returns its id and whether it is new.
     */
    std::pair<int, bool> insert(const uint64_t *bits) {
        uint32_t h = uint32_t(hash_words(bits, num_words));
        size_t pos = h & mask;
        while (true) {
            int id = table[pos];
            if (id < 0)
                break;
            if (state_hashes[id] == h &&
                std::memcmp(get(id), bits, sizeof(uint64_t) * num_words) == 0)
                return {id, false};
            pos = (pos + 1) & mask;
        }
        int id = num_states++;
        if ((id >> BLOCK_BITS) >= int(blocks.size()))
            blocks.emplace_back(new uint64_t[size_t(BLOCK_SIZE) * num_words]);
        std::memcpy(const_cast<uint64_t *>(get(id)), bits, sizeof(uint64_t) * num_words);
        state_hashes.push_back(h);
        table[pos] = id;
        if (size_t(num_states) * 2 > table.size())
            grow();
        return {id, true};
    }

    size_t memory_usage_bytes() const {
        return blocks.size() * size_t(BLOCK_SIZE) * num_words * sizeof(uint64_t)
               + table.size() * sizeof(int) + state_hashes.capacity() * sizeof(uint32_t);
    }
};

#endif  // SEARCH_GROUNDED_GROUND_STATE_REGISTRY_H
