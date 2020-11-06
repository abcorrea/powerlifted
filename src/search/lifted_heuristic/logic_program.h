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
    std::unordered_map<std::string, int> map_atom_to_index;
    std::unordered_map<std::string, int> map_object_to_index;

public:
    LogicProgram() = default;

    LogicProgram(std::vector<Fact> &&f,
                 std::vector<Object> &&o,
                 std::vector<std::unique_ptr<RuleBase>> &&r,
                 std::unordered_map<int, std::string> &&m,
                 std::unordered_map<std::string, int> &&a_to_i,
                 std::unordered_map<std::string, int> &&o_to_i)
        : facts(std::move(f)),
          objects(std::move(o)),
          rules(std::move(r)),
          map_index_to_atom(std::move(m)),
          map_atom_to_index(std::move(a_to_i)),
          map_object_to_index(std::move(o_to_i)){}


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

    int get_atom_by_name(const std::string &name) const;

    int get_object_by_name(const std::string &name) const;

    size_t get_number_of_facts();

    void clean_rule(int r) {
        rules[r].reset();
    }

    void update_fact_cost(int fact, int cost);

    void reset_facts(size_t i);
};

}

#endif
