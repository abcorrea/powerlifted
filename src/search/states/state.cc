
#include "state.h"

#include <boost/functional/hash.hpp>

using namespace std;

void DBState::add_tuple(int relation, const GroundAtom &args) {
  relations[relation].tuples.insert(args);
}


std::size_t hash_value(const DBState &s) {
    std::size_t seed = 0;
    for (bool b : s.nullary_atoms) {
        boost::hash_combine(seed, b);
    }
    for (const Relation &r : s.relations) {
        std::vector<std::size_t> x;
        for (const GroundAtom &vga : r.tuples) {
            std::size_t aux_seed = vga.size();
            for (auto &i : vga)
                aux_seed ^= i + 0x9e3779b9 + (aux_seed << 6) + (aux_seed >> 2);
            x.push_back(aux_seed);
            //boost::hash_combine(seed, vga);
        }
        std::sort(x.begin(), x.end());
        for (std::size_t e : x) {
            boost::hash_combine(seed, e);
        }
    }
    return seed;
}
