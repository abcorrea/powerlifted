#ifndef SEARCH_STATES_GROUND_STATE_H
#define SEARCH_STATES_GROUND_STATE_H

#include <cstdint>
#include <vector>

/*
  States of the ground task (see GroundTask) are bitsets over its facts,
  stored as arrays of 64-bit words.
*/
namespace ground_state {
inline int get_num_words(int num_facts) { return num_facts > 0 ? (num_facts + 63) / 64 : 1; }

inline bool test_bit(const uint64_t *bits, int fact)
{
    return (bits[fact >> 6] >> (fact & 63)) & 1ULL;
}

inline void set_bit(uint64_t *bits, int fact) { bits[fact >> 6] |= 1ULL << (fact & 63); }

inline void reset_bit(uint64_t *bits, int fact) { bits[fact >> 6] &= ~(1ULL << (fact & 63)); }

//! Set `facts` to the facts true in the state, in increasing order.
inline void get_true_facts(const uint64_t *bits, int num_words, std::vector<int> &facts)
{
    facts.clear();
    for (int w = 0; w < num_words; ++w) {
        uint64_t word = bits[w];
        while (word) {
            facts.push_back(w * 64 + __builtin_ctzll(word));
            word &= word - 1;
        }
    }
}

//! Append the facts true in `to` but not in `from` to `added`, and the facts
//! true in `from` but not in `to` to `removed`.
inline void get_changes(const uint64_t *from,
                        const uint64_t *to,
                        int num_words,
                        std::vector<int> &added,
                        std::vector<int> &removed)
{
    for (int w = 0; w < num_words; ++w) {
        uint64_t word = to[w] & ~from[w];
        while (word) {
            added.push_back(w * 64 + __builtin_ctzll(word));
            word &= word - 1;
        }
        word = from[w] & ~to[w];
        while (word) {
            removed.push_back(w * 64 + __builtin_ctzll(word));
            word &= word - 1;
        }
    }
}
}  // namespace ground_state

#endif  // SEARCH_STATES_GROUND_STATE_H
