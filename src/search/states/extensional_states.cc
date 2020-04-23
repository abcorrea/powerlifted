
#include "extensional_states.h"
#include "../task.h"
#include "../utils.h"
#include "../utils/cartesian_iterator.h"
#include "../utils/hash.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>


std::size_t ExtensionalPackedState::Hash::operator() (const ExtensionalPackedState &s) const {
    return boost::hash_range(s.atoms.cbegin(), s.atoms.cend());
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

        for (auto it = utils::cartesian_iterator(objects_per_arg); !it.ended(); ++it) {
            const std::vector<int>& args = *it;
            ati_map.emplace(args, index_to_args.size());
            index_to_args.emplace_back(pid, args);
        }

        // Looks a bit redundant, but that's the way it is:
        blank_state.relations[pid].predicate_symbol = pid;
    }

    std::cout << "Indexed a total of " << num_atoms() << " atoms" << std::endl;
}

unsigned ExtensionalStatePacker::to_index(int predicate, const std::vector<int>& arguments) const {
    assert(0 <= predicate && (unsigned) predicate <= args_to_index.size());
    return args_to_index[predicate].at(arguments);
}


ExtensionalPackedState ExtensionalStatePacker::pack_state(const State &state) const {
    ExtensionalPackedState packed(num_atoms());

    // state.nullary_atoms contains one element per predicate index, regardless of whether
    // the predicate is nullary or not
    for (std::size_t i = 0, sz = state.nullary_atoms.size(); i < sz; ++i) {
        if (state.nullary_atoms[i]) {
            packed.atoms[to_index(i, {})] = true;
        }
    }

    for (const Relation &relation:state.relations) {
        int pid = relation.predicate_symbol;
        for (const auto &tuple:relation.tuples) {
            packed.atoms[to_index(pid, tuple)] = true;
        }
    }
    return packed;
}

State ExtensionalStatePacker::unpack_state(const ExtensionalPackedState &packed) const {
    State result(blank_state);  // Let's start off with the precomputed state

    auto natoms = packed.atoms.size();
    assert(natoms == num_atoms());
    for (std::size_t aid = 0; aid < natoms; ++aid) {
        if (!packed.atoms[aid]) continue;

        const auto& x = index_to_args[aid];
        int pid = x.first;
        const args_t& args = x.second;
        if (args.empty()) {  // A nullary predicate
            // Make true the position corresponding to *the predicate*
            result.nullary_atoms[pid] = true;

        } else {  // An arity > 0 predicate
            result.relations[pid].tuples.insert(args);
        }
    }

    return result;
}
