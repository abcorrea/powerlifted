#ifndef GROUNDER__LOGIC_PROGRAM_H_
#define GROUNDER__LOGIC_PROGRAM_H_

#include "fact.h"
#include "object.h"
#include "rules/rule_base.h"

#include <deque>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lifted_heuristic {

typedef std::unordered_set<Arguments, HashArguments> FactBucket;

class LogicProgram {
    std::vector<Fact> facts;
    std::vector<Object> objects;
    std::vector<std::unique_ptr<RuleBase>> rules;
    std::unordered_map<int, std::string> map_index_to_atom;

public:
    LogicProgram() = default;

    LogicProgram(std::vector<Fact> &&f,
                 std::vector<Object> &&o,
                 std::vector<std::unique_ptr<RuleBase>> &&r,
                 std::unordered_map<int, std::string> &&m)
        : facts(std::move(f)),
          objects(std::move(o)),
          rules(std::move(r)),
          map_index_to_atom(std::move(m)) {}

    void insert_fact(Fact &f);

    const std::vector<Fact> &get_facts() const;

    const std::vector<std::unique_ptr<RuleBase>> &get_rules() const;

    const std::vector<Object> &get_objects() const {
        return objects;
    }

    const std::unordered_map<int, std::string> &get_map_index_to_atom() const {
        return map_index_to_atom;
    }

    RuleBase &get_rule_by_index(int index);

    const Fact &get_fact_by_index(int index) const;

    const std::string &get_atom_by_index(int index) const;

    size_t get_number_of_facts();

    void clean_rule(int r) {
        rules[r].reset();
    }

};

}

#endif
