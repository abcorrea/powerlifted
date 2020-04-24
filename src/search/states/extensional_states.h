#ifndef EXTENSIONAL_SEARCH_STATE_PACKER_H
#define EXTENSIONAL_SEARCH_STATE_PACKER_H

#include <boost/functional/hash.hpp>

#include "state.h"
#include <unordered_map>
#include <vector>

class Task;

/**
 * @brief A bitvector-based representation of states
 *
 * @details
 *
 */

class ExtensionalStatePacker;

class ExtensionalPackedState {
public:
    using StatePackerT = ExtensionalStatePacker;

    std::vector<bool> atoms;

    explicit ExtensionalPackedState(std::size_t size) : atoms(size, false) {}

    ExtensionalPackedState(const ExtensionalPackedState&) = default;
    ExtensionalPackedState(ExtensionalPackedState&&) = default;
    ExtensionalPackedState& operator=(const ExtensionalPackedState&) = delete;
    ExtensionalPackedState& operator=(ExtensionalPackedState&&) = delete;

    bool operator==(const ExtensionalPackedState &b) const { return atoms == b.atoms; }

    struct Hash {
        std::size_t operator() (const ExtensionalPackedState &s) const;
    };

    using HashT = Hash;
};


/**
 * @brief Pack and unpack states into a more compact representation
 */
class ExtensionalStatePacker {
protected:
    const Task &task;

    std::size_t npreds;

    using args_t = std::vector<int>;
    std::vector<std::unordered_map<args_t, unsigned, boost::hash<args_t>>> args_to_index;
    std::vector<std::pair<int, args_t>> index_to_args;

    //! A state placeholder for faster creation of states in ExtensionalStatePacker::pack_state
    State blank_state;


public:
    explicit ExtensionalStatePacker(const Task &task);

    std::size_t num_atoms() const { return index_to_args.size(); }

    unsigned to_index(int predicate, const std::vector<int>& arguments) const;

    ExtensionalPackedState pack_state(const State &state) const;

    State unpack_state(const ExtensionalPackedState &packed) const;
};

#endif // EXTENSIONAL_SEARCH_STATE_PACKER_H
