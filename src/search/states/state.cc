
#include "state.h"

#include "../utils/hash.h"

using namespace std;

void DBState::add_tuple(int relation, const GroundAtom &args)
{
    relations[relation].tuples.insert(args);
}


std::size_t hash_value(const DBState &s)
{
    utils::HashState hash_state;
    for (bool b : s.nullary_atoms) {
        utils::feed(hash_state, static_cast<unsigned int>(b));
    }
    for (const Relation &r : s.relations) {
        // Use addition (commutative) to combine per-atom hashes so that
        // iteration order over the unordered_set does not matter.  O(n)
        // instead of the previous O(n log n) sort.
        std::size_t relation_hash = 0;
        for (const GroundAtom &ga : r.tuples) {
            relation_hash += utils::get_hash(ga);
        }
        utils::feed(hash_state, static_cast<std::uint64_t>(relation_hash));
    }
    return hash_state.get_hash64();
}
