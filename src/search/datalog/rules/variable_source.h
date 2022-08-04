#ifndef SEARCH_DATALOG_RULES_VARIABLE_SOURCE_H_
#define SEARCH_DATALOG_RULES_VARIABLE_SOURCE_H_

#include "../datalog_atom.h"

#include <unordered_map>
#include <vector>

namespace datalog {

class VariableSource {

    // Where to get the variable from:
    // Entry i is the ith variable of the action schema.
    // If the first element is negative, then it indicates the atom in the body where this variable
    // can be retrieved. If its positive, it indicates the auxiliary body atom that can query this
    // variable. The second element indicates the position of the variable in the rule atom or in the
    // auxiliary atom variables.
    // Attention: if it is negative, we need to reduce 1 after transforming to positive (to avoid ambiguity with 0)

    std::vector<std::pair<int, int>> table;

    // TODO Check if this is a bottleneck and perhaps do it locally during the split
    std::unordered_map<int, int> map_term_to_entry;
    std::vector<int> map_entry_to_term;

public:
    VariableSource(const DatalogAtom &effect, const std::vector<DatalogAtom> &conditions) {
        table.clear();

        std::vector<Term> all_arguments;
        for (Term t: effect.get_arguments()) {
            if (t.is_object()) continue;
            all_arguments.emplace_back(t);
        }
        for (const DatalogAtom &b : conditions) {
            for (const Term &t: b.get_arguments()) {
                if (t.is_object()) continue;
                if (std::find(all_arguments.begin(), all_arguments.end(), t) == all_arguments.end()) {
                    all_arguments.emplace_back(t);
                }
            }
        }


        for (const Term &arg: all_arguments) {
            if (arg.is_object()) continue;
            int term_index = arg.get_index();
            map_term_to_entry.insert(std::make_pair(term_index, table.size()));
            map_entry_to_term.push_back(term_index);
            std::pair<int, int> p;
            bool found_correspondence = false;
            int body_atom_counter = 0;
            for (const DatalogAtom &b : conditions) {
                int position_counter = 0;
                for (const Term &body_arg: b.get_arguments()) {
                    if (body_arg.is_object()) continue;
                    if (body_arg.get_index()==arg.get_index()) {
                        p.first = (body_atom_counter + 1)*-1;
                        p.second = position_counter;
                        found_correspondence = true;
                        break;
                    }
                    position_counter++;
                }
                if (found_correspondence) break;
                body_atom_counter++;
            }
            table.emplace_back(p);
        }
    }

    std::vector<std::pair<int, int>> &get_table() {
        return table;
    }

    int get_term_from_table_entry_index(int i) const {
        return map_entry_to_term[i];
    }

    int get_table_entry_index_from_term(int i) const {
        return map_term_to_entry.at(i);
    }

    int get_position_of_atom_in_same_body_rule(int i) const {
        if (i < 0)
            return (i*-1)-1;
        else
            return i;
    }

    bool is_variable_found_in_body(int i) const {
        return table[i].first < 0;
    }

    std::vector<std::pair<int, int>> get_table() const {
        return table;
    }

    void output() const {
        int v = 0;
        for (const std::pair<int, int> &p : table) {
            bool is_found_in_rule = (p.first < 0);
            int position_in_body = p.second;
            int body_position = get_position_of_atom_in_same_body_rule(p.first);
            if (is_found_in_rule) {
                std::cout << "?v" << map_entry_to_term[v] << ' ' << "DIRECT " << body_position << ' ' << position_in_body << std::endl;
            }
            else {
                std::cout << "?v" << map_entry_to_term[v] << ' ' << "INDIRECT " << body_position << ' ' << position_in_body << std::endl;
            }
            ++v;
        }
    }

    void update_ith_entry(int i, int first, int second) {
        assert(i < int(table.size()));
        table[i] = std::make_pair(first, second);
    }

    void add_entry(int term_index, int first, int second) {
        table.emplace_back(first, second);
        map_term_to_entry.insert(std::make_pair(term_index, table.size()));
        map_entry_to_term.push_back(term_index);
    }

};

}

#endif //SEARCH_DATALOG_RULES_VARIABLE_SOURCE_H_
