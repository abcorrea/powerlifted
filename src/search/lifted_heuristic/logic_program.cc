#include "logic_program.h"

#include <vector>

using namespace std;

namespace lifted_heuristic {

void LogicProgram::insert_fact(Fact &f) {
    facts.push_back(f);
}

const vector<Fact> &LogicProgram::get_facts() const {
    return facts;
}

const vector<unique_ptr<RuleBase>> &LogicProgram::get_rules() const {
    return rules;
}

RuleBase &LogicProgram::get_rule_by_index(int index) {
    return *rules[index];
}

const Fact &LogicProgram::get_fact_by_index(int index) const {
    return facts[index];
}

size_t LogicProgram::get_number_of_facts() {
    return facts.size();
}

const std::string &LogicProgram::get_atom_by_index(int index) const {
    assert(map_index_to_atom.find(index)!=map_index_to_atom.end());
    return map_index_to_atom.at(index);
}

int LogicProgram::get_atom_by_name(const std::string &name) const {
    assert(map_atom_to_index.find(name) != map_atom_to_index.end());
    return map_atom_to_index.at(name);
}

void LogicProgram::reset_facts(size_t i) {
    assert(facts.size() >= i);
    facts.erase(facts.begin()+i, facts.end());
}

int LogicProgram::get_object_by_name(const std::string &name) const {
    return map_object_to_index.at(name);
}

}