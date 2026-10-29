#ifndef SEARCH_GROUNDED_GROUND_STATE_REGISTRY_H
#define SEARCH_GROUNDED_GROUND_STATE_REGISTRY_H

#include <cstdint>
#include <algorithm>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

/**
 * @brief Stores ground states as fixed-size bitsets (one bit per fact) in
 * blocks of contiguous memory, and indexes them with an open-addressing hash
 * table of state ids.
 *
 * In compact mode (for tasks with many facts), each state is stored in an
 * arena as whichever is smaller: the sorted list of its true facts or its
 * bitset (as 32-bit halves). The choice depends only on the state, so equal
 * states have equal representations.
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

    bool sparse;
    // Compact mode: the representation of state i is
    // sparse_facts[sparse_start[i] .. sparse_start[i + 1]), a fact list if
    // is_list[i], otherwise the bitset words split into 32-bit halves.
    std::vector<uint32_t> sparse_facts;
    std::vector<size_t> sparse_start;
    std::vector<char> is_list;
    std::vector<uint32_t> scratch;

    //! Compact representation of the state (in scratch); true if a list.
    bool encode(const uint64_t *bits) {
        scratch.clear();
        size_t count = 0;
        for (int w = 0; w < num_words; ++w)
            count += __builtin_popcountll(bits[w]);
        if (count < 2 * size_t(num_words)) {
            for (int w = 0; w < num_words; ++w) {
                uint64_t word = bits[w];
                while (word) {
                    scratch.push_back(w * 64 + __builtin_ctzll(word));
                    word &= word - 1;
                }
            }
            return true;
        }
        for (int w = 0; w < num_words; ++w) {
            scratch.push_back(uint32_t(bits[w]));
            scratch.push_back(uint32_t(bits[w] >> 32));
        }
        return false;
    }

    static uint64_t hash_ints(const uint32_t *values, size_t n) {
        uint64_t h = 0x9E3779B97F4A7C15ULL ^ uint64_t(n);
        for (size_t i = 0; i < n; ++i)
            h ^= uint64_t(values[i]) + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
        h ^= h >> 33;
        h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 33;
        return h;
    }

    std::pair<int, bool> insert_sparse(const uint64_t *bits) {
        bool list = encode(bits);
        uint32_t h = uint32_t(hash_ints(scratch.data(), scratch.size()) ^ list);
        size_t pos = h & mask;
        while (true) {
            int id = table[pos];
            if (id < 0)
                break;
            if (state_hashes[id] == h && is_list[id] == list &&
                sparse_start[id + 1] - sparse_start[id] == scratch.size() &&
                std::equal(scratch.begin(), scratch.end(),
                           sparse_facts.begin() + sparse_start[id]))
                return {id, false};
            pos = (pos + 1) & mask;
        }
        int id = num_states++;
        sparse_facts.insert(sparse_facts.end(), scratch.begin(), scratch.end());
        sparse_start.push_back(sparse_facts.size());
        is_list.push_back(list);
        state_hashes.push_back(h);
        table[pos] = id;
        if (size_t(num_states) * 2 > table.size())
            grow();
        return {id, true};
    }

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
    //! Compact mode for tasks with more than 4096 facts.
    explicit GroundStateRegistry(int num_facts)
        : num_words((num_facts + 63) / 64), num_states(0), table(1 << 16, -1),
          mask((1 << 16) - 1),
          sparse(num_facts > 4096) {
        if (num_words == 0)
            num_words = 1;
        sparse_start.push_back(0);
    }

    bool is_sparse() const {
        return sparse;
    }

    //! Write the bitset of state id to `bits` (num_words words).
    void copy_to(int id, uint64_t *bits) const {
        if (!sparse) {
            std::memcpy(bits, get(id), sizeof(uint64_t) * num_words);
            return;
        }
        const uint32_t *data = sparse_facts.data() + sparse_start[id];
        if (!is_list[id]) {
            for (int w = 0; w < num_words; ++w)
                bits[w] = uint64_t(data[2 * w]) | (uint64_t(data[2 * w + 1]) << 32);
            return;
        }
        std::memset(bits, 0, sizeof(uint64_t) * num_words);
        for (size_t i = 0; i < sparse_start[id + 1] - sparse_start[id]; ++i)
            bits[data[i] >> 6] |= 1ULL << (data[i] & 63);
    }

    int get_num_words() const {
        return num_words;
    }

    int size() const {
        return num_states;
    }

    //! Bitset of state id (dense mode only; use copy_to otherwise).
    const uint64_t *get(int id) const {
        return blocks[id >> BLOCK_BITS].get() + size_t(id & (BLOCK_SIZE - 1)) * num_words;
    }

    /**
     * Insert the state (if new). Returns its id and whether it is new.
     */
    std::pair<int, bool> insert(const uint64_t *bits) {
        if (sparse)
            return insert_sparse(bits);
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
               + sparse_facts.capacity() * sizeof(uint32_t) + is_list.capacity()
               + sparse_start.capacity() * sizeof(size_t)
               + table.size() * sizeof(int) + state_hashes.capacity() * sizeof(uint32_t);
    }
};

#endif  // SEARCH_GROUNDED_GROUND_STATE_REGISTRY_H
