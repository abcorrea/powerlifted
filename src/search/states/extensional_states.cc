
#include "extensional_states.h"
#include "../algorithms/cartesian_iterator.h"
#include "../task.h"
#include "../utils.h"
#include "../utils/hash.h"

#include <cstdint>
#include <iostream>
#include <vector>


unsigned ExtensionalPackedState::Hash::operator() (const ExtensionalPackedState &s) const {

//    std::size_t seed = 0;
//    boost::hash_combine(seed, s.atoms.m_num_bits);
//    boost::hash_combine(seed, s.atoms.m_bits);
//    return (unsigned) seed;
//    return (unsigned) boost::hash_value(s.atoms);
//    return (unsigned) boost::hash_range(s.atoms.cbegin(), s.atoms.cend());
//    std::hash<std::vector<bool>> hasher;
//    hasher.operator()(s.atoms);
//    return (unsigned) hasher(s.atoms);
    utils::HashState hash_state;
    utils::feed(hash_state, s.atoms);
    return hash_state.get_hash32();
}


ExtensionalStatePacker::ExtensionalStatePacker(const Task &task) :
    task(task), npreds(task.predicates.size()), args_to_index(), index_to_args(), blank_state(npreds)
{
    auto objects_per_type = task.compute_object_index();
    args_to_index.reserve(npreds);

    for (std::size_t pid = 0; pid < npreds; ++pid) {
        const auto& pred = task.predicates[pid];

        args_to_index.emplace_back();  // emplace back an empty map
        auto& ati_map = args_to_index.back();

        const auto& types = pred.getTypes();
        if (types.empty()) { // A nullary predicate - special treatment
            ati_map.emplace(args_t(), index_to_args.size());
            index_to_args.push_back({pid, {}});
            continue;
        }

        std::vector<std::vector<int>> objects_per_arg;
        for (const auto& type:types) {
            objects_per_arg.push_back(objects_per_type[type]);
        }

        for (auto it = utils::cartesian_iterator<int>(objects_per_arg); !it.ended(); ++it) {
            const std::vector<int>& args = *it;
            ati_map.emplace(args, index_to_args.size());
            index_to_args.emplace_back(pid, args);
        }

        // Looks a bit redundant, but that's the way it is:
        blank_state.set_relation_predicate_symbol(pid, pid);
    }

    std::cout << "Indexed a total of " << num_atoms() << " atoms" << std::endl;
}

unsigned ExtensionalStatePacker::to_index(int predicate, const std::vector<int>& arguments) const {
    assert(0 <= predicate && (unsigned) predicate <= args_to_index.size());
    return args_to_index[predicate].at(arguments);
}


ExtensionalPackedState ExtensionalStatePacker::pack(const DBState &state) const {
    ExtensionalPackedState packed(num_atoms());

    // state.nullary_atoms contains one element per predicate index, regardless of whether
    // the predicate is nullary or not
    const auto& nullary_atoms = state.get_nullary_atoms();
    for (std::size_t i = 0, sz = nullary_atoms.size(); i < sz; ++i) {
        if (nullary_atoms[i]) {
//            packed.atoms[to_index(i, {})] = true;
            packed.atoms.set(to_index(i, {}));
        }
    }

    for (const Relation &relation:state.get_relations()) {
        int pid = relation.predicate_symbol;
        for (const auto &tuple:relation.tuples) {
//            packed.atoms[to_index(pid, tuple)] = true;
            packed.atoms.set(to_index(pid, tuple));
        }
    }
    return packed;
}

DBState ExtensionalStatePacker::unpack(const ExtensionalPackedState &packed) const {
    DBState result(blank_state);  // Let's start off with the precomputed state

    auto natoms = packed.atoms.size();
    assert(natoms == num_atoms());
    for (std::size_t aid = 0; aid < natoms; ++aid) {
        if (!packed.atoms[aid]) continue;

        const auto& x = index_to_args[aid];
        int pid = x.first;
        const args_t& args = x.second;
        if (args.empty()) {  // A nullary predicate
            // Make true the position corresponding to *the predicate*
            result.set_nullary_atom(pid, true);

        } else {  // An arity > 0 predicate
            result.insert_tuple_in_relation(args, pid);
        }
    }

    return result;
}
