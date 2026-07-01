#ifndef GROUNDER_RULES_JOIN_RULE_H
#define GROUNDER_RULES_JOIN_RULE_H

#include "rule_base.h"

#include "../datalog_fact.h"

#include "../../parallel_hashmap/phmap.h"

#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../../utils/hash.h"
#include "../../utils/small_vector.h"

namespace datalog {

// Join keys hold one entry per joining variable — almost always 1 or 2. Keeping
// them in a small-buffer-optimized vector avoids a heap allocation per distinct
// key stored in the join hash maps and removes the pointer indirection when
// hashing/comparing keys, which happens on every join hash-table access (the
// hottest path in the grounder). Content-based hash and element-wise operator==
// are identical to the previous std::vector<int>, so the maps behave the same.
using JoinHashKey = utils::small_vector<int, 2>;

struct JoinHashKeyHash {
    std::size_t operator()(const JoinHashKey &v) const {
        return utils::hash_range(v.begin(), v.end());
    }
};

class JoinHashEntry {

    phmap::flat_hash_set<Fact> entry;

public:
    JoinHashEntry() = default;

    void insert(const Fact &f) { entry.insert(f); }

    phmap::flat_hash_set<Fact>::const_iterator begin() const { return entry.begin(); }

    phmap::flat_hash_set<Fact>::const_iterator end() const { return entry.end(); }
};

class JoinHashTable {
    phmap::flat_hash_map<JoinHashKey, JoinHashEntry, JoinHashKeyHash> hash_table_1;
    phmap::flat_hash_map<JoinHashKey, JoinHashEntry, JoinHashKeyHash> hash_table_2;

    static bool valid_position(size_t i) { return (i == 0 or i == 1); }

public:
    JoinHashTable() = default;

    void insert(const Fact &f, const JoinHashKey &key, int position)
    {
        assert(valid_position(position));
        if (position == 0) {
            //            hash_table_1.emplace(key, JoinHashEntry()); // redundant
            hash_table_1[key].insert(f);
        }
        else {
            //            hash_table_2.emplace(key, JoinHashEntry()); // redundant
            hash_table_2[key].insert(f);
        }
    }

    const JoinHashEntry &get_entries(const JoinHashKey &key, size_t position)
    {
        assert(valid_position(position));
        // Look up partners with find(), not operator[]: a miss (a popped fact
        // whose join key has no partner yet — very common) must not insert a
        // spurious empty entry, which would copy the vector<int> key into the map
        // and possibly rehash it. An absent key yields the same empty range.
        static const JoinHashEntry empty_entry;
        const auto &table = (position == 0) ? hash_table_1 : hash_table_2;
        const auto it = table.find(key);
        return (it == table.end()) ? empty_entry : it->second;
    }

    // Empty both tables but keep their bucket arrays so the next grounding
    // reuses them instead of reallocating. Equivalent to assigning a fresh
    // JoinHashTable, only without the malloc/free churn (clean_up() runs once
    // per join rule per state evaluation).
    void clear()
    {
        hash_table_1.clear();
        hash_table_2.clear();
    }
};

class JoiningVariables {
    // Each vector indicates the positions of the free variables
    // in the key occur in each respective rule of the body. If the first element
    // has X in it's 3rd position, then it means that the first rule of the body
    // has the third variable of the key in its Xth position.
    std::vector<int> positions_0;
    std::vector<int> positions_1;
    size_t number_of_joining_vars;

public:
    explicit JoiningVariables(const std::vector<DatalogAtom> &conditions)
    {
        if (conditions.size() != 2) {
            number_of_joining_vars = 0;
            return;
        }

        std::vector<int> new_key;

        int pos1 = 0;
        for (const Term &term : conditions[0].get_arguments()) {
            int c = term.get_index();
            auto it2 = std::find(
                conditions[1].get_arguments().begin(), conditions[1].get_arguments().end(), term);
            if (it2 != conditions[1].get_arguments().end()) {
                // Free variables match in both atoms
                int pos2 = std::distance(conditions[1].get_arguments().begin(), it2);
                new_key.push_back(c);
                positions_0.push_back(pos1);
                positions_1.push_back(pos2);
            }
            ++pos1;
        }
        number_of_joining_vars = new_key.size();
    }

    size_t get_number_of_joining_vars() const { return number_of_joining_vars; }

    const std::vector<int> &get_joining_vars_of_condition(int condition) const
    {
        assert(condition == 0 or condition == 1);
        if (condition == 0) {
            return positions_0;
        }
        else {
            return positions_1;
        }
    }
};

class JoinRule : public RuleBase {
    JoinHashTable hash_table_indices;
    JoiningVariables position_of_joining_vars;

public:
    JoinRule(int weight,
             DatalogAtom eff,
             std::vector<DatalogAtom> c,
             std::unique_ptr<Annotation> annotation)
        : RuleBase(weight, std::move(eff), std::move(c), std::move(annotation)),
          position_of_joining_vars(conditions)
    {
    }

    void clean_up() override { hash_table_indices.clear(); }

    int get_type() const override { return JOIN; }

    void insert_fact_in_hash(const Fact &fact, const JoinHashKey &key, int position)
    {
        hash_table_indices.insert(fact, key, position);
    }

    const JoinHashEntry &get_facts_matching_key(const JoinHashKey &key, int position)
    {
        return hash_table_indices.get_entries(key, position);
    }

    const std::vector<int> &get_position_of_matching_vars(int condition) const
    {
        return position_of_joining_vars.get_joining_vars_of_condition(condition);
    }

    size_t get_number_joining_vars() const
    {
        return position_of_joining_vars.get_number_of_joining_vars();
    }

    int get_inverse_position(int i) const { return (i + 1) % 2; }

    std::string get_type_name() override { return "JoinRule"; }
};

}  // namespace datalog

#endif  // GROUNDER_RULES_JOIN_RULE_H
