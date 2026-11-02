#include "ground_state_registry.h"

#include "ground_state.h"

#include <algorithm>
#include <cstring>

using namespace std;

namespace {
// Tasks with more facts than this store their states compactly.
const int MIN_FACTS_FOR_COMPACT_STATES = 4096;

const size_t INITIAL_TABLE_SIZE = 1 << 16;

template <typename T>
uint32_t hash_values(const T *values, size_t n)
{
    uint64_t h = 0x9E3779B97F4A7C15ULL ^ uint64_t(n);
    for (size_t i = 0; i < n; ++i)
        h ^= uint64_t(values[i]) + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;
    return uint32_t(h);
}
}  // namespace

GroundStateRegistry::GroundStateRegistry(int num_facts)
    : num_words(ground_state::get_num_words(num_facts)),
      num_states(0),
      table(INITIAL_TABLE_SIZE, -1),
      mask(INITIAL_TABLE_SIZE - 1),
      compact(num_facts > MIN_FACTS_FOR_COMPACT_STATES)
{
    compact_start.push_back(0);
}

uint64_t *GroundStateRegistry::get_bits(int id)
{
    return blocks[id >> BLOCK_BITS].get() + size_t(id & (BLOCK_SIZE - 1)) * num_words;
}

const uint64_t *GroundStateRegistry::get_bits(int id) const
{
    return blocks[id >> BLOCK_BITS].get() + size_t(id & (BLOCK_SIZE - 1)) * num_words;
}

bool GroundStateRegistry::encode(const uint64_t *bits)
{
    encoded.clear();
    size_t num_true = 0;
    for (int w = 0; w < num_words; ++w)
        num_true += __builtin_popcountll(bits[w]);
    if (num_true < 2 * size_t(num_words)) {
        for (int w = 0; w < num_words; ++w) {
            uint64_t word = bits[w];
            while (word) {
                encoded.push_back(w * 64 + __builtin_ctzll(word));
                word &= word - 1;
            }
        }
        return true;
    }
    for (int w = 0; w < num_words; ++w) {
        encoded.push_back(uint32_t(bits[w]));
        encoded.push_back(uint32_t(bits[w] >> 32));
    }
    return false;
}

bool GroundStateRegistry::is_equal(int id, uint32_t hash, const uint64_t *bits, bool list) const
{
    if (state_hashes[id] != hash)
        return false;
    if (!compact)
        return memcmp(get_bits(id), bits, sizeof(uint64_t) * num_words) == 0;
    return is_list[id] == list && compact_start[id + 1] - compact_start[id] == encoded.size() &&
           equal(encoded.begin(), encoded.end(), compact_data.begin() + compact_start[id]);
}

void GroundStateRegistry::grow()
{
    size_t new_size = table.size() * 2;
    vector<int> new_table(new_size, -1);
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

void GroundStateRegistry::copy_to(int id, uint64_t *bits) const
{
    if (!compact) {
        memcpy(bits, get_bits(id), sizeof(uint64_t) * num_words);
        return;
    }
    const uint32_t *data = compact_data.data() + compact_start[id];
    if (!is_list[id]) {
        for (int w = 0; w < num_words; ++w)
            bits[w] = uint64_t(data[2 * w]) | (uint64_t(data[2 * w + 1]) << 32);
        return;
    }
    memset(bits, 0, sizeof(uint64_t) * num_words);
    for (size_t i = 0; i < compact_start[id + 1] - compact_start[id]; ++i)
        ground_state::set_bit(bits, data[i]);
}

pair<int, bool> GroundStateRegistry::insert(const uint64_t *bits)
{
    bool list = false;
    uint32_t hash;
    if (compact) {
        list = encode(bits);
        hash = hash_values(encoded.data(), encoded.size()) ^ list;
    }
    else {
        hash = hash_values(bits, num_words);
    }
    size_t pos = hash & mask;
    for (; table[pos] >= 0; pos = (pos + 1) & mask) {
        if (is_equal(table[pos], hash, bits, list))
            return {table[pos], false};
    }

    int id = num_states++;
    if (compact) {
        compact_data.insert(compact_data.end(), encoded.begin(), encoded.end());
        compact_start.push_back(compact_data.size());
        is_list.push_back(list);
    }
    else {
        if ((id >> BLOCK_BITS) >= int(blocks.size()))
            blocks.emplace_back(new uint64_t[size_t(BLOCK_SIZE) * num_words]);
        memcpy(get_bits(id), bits, sizeof(uint64_t) * num_words);
    }
    state_hashes.push_back(hash);
    table[pos] = id;
    if (size_t(num_states) * 2 > table.size())
        grow();
    return {id, true};
}
