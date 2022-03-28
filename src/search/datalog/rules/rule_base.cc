#include "rule_base.h"

#include "join.h"

#include "../datalog.h"

#include <algorithm>

using namespace std;

namespace datalog {

int RuleBase::next_index = 0;


void RuleBase::output_variable_table() const {
    variable_source.output();

}

void RuleBase::update_conditions(DatalogAtom new_atom,
                                 const std::vector<DatalogAtom> &new_rule_conditions,
                                 const VariableSource &variable_source_new_rule,
                                 std::vector<size_t> &&body_ids) {

    /*
     * TODO Change removal method and create local lookup table to simplify the decrement of
     * indices.
     */

    std::sort(body_ids.begin(), body_ids.end());
    std::vector<DatalogAtom> old_conditions = conditions;
    for (int i = int(body_ids.size()) -1; i >= 0; --i) {
        conditions.erase(conditions.begin() + body_ids[i]);
    }
    conditions.push_back(new_atom);

    int final_index = conditions.size() - 1;
    int entry_counter = 0;
    for (std::pair<int, int> &p : variable_source.get_table()) {
        int canonical_index = p.first;
        if (canonical_index < 0)
            canonical_index = (canonical_index*-1)-1; // Conversion to positive index

        if (std::find(body_ids.begin(), body_ids.end(), canonical_index) != body_ids.end()) {
            // First, we find out which term is this entry p referring to
            int term_index = variable_source.get_term_from_table_entry_index(entry_counter);

            p.first = final_index; // Now this points to the final conditions of the rule

            // Then, we find out the position of its entry in the table
            bool found_correspondence = false;
            for (const DatalogAtom &b : new_rule_conditions) {
                for (const Term &body_arg : b.get_arguments()) {
                    if (body_arg.is_object()) continue;
                    if (body_arg.get_index() == term_index) {
                        p.second = variable_source_new_rule.get_table_entry_index_from_term(term_index);
                        found_correspondence = true;
                        break;
                    }
                }
                if (found_correspondence) break;
            }
        }
        else {
            for (int i = int(body_ids.size())-1; i >= 0; --i) {
                /*
                 * There are two types of entries in the variable source table:
                 *
                 * 1. Negative: it means that we can query the value of the corresponding variable
                 * directly from the body; and
                 * 2. Positive (or zero): it means that we need to backtrack through the achiever
                 * of the atom it points to.
                 *
                 * We update (i.e., change the entry to one position ahead) accordingly, depending on
                 * whether it is a positive or a negative value.
                 */
                if (p.first < 0) {
                    if (canonical_index > int(body_ids[i]))
                        p.first = p.first + 1; // Case where p.first is negative, so we increment 1
                }
                else {
                    if (p.first > int(body_ids[i]))
                        p.first = p.first - 1;
                }
            }
        }
        entry_counter++;
    }

}

void RuleBase::set_specific_condition(size_t i, DatalogAtom atom) {
    conditions[i] = atom;
}

void RuleBase::update_single_condition_and_variable_source_table(size_t j, DatalogAtom atom)  {

    std::vector<int> argument_shift(conditions[j].get_arguments().size(), 0);
    int shift = 0;
    for (const Term &t : atom.get_arguments()) {
        if (t.is_object()) {
            shift++;
            continue;
        }
        argument_shift[t.get_index()] = shift;
    }

    conditions[j] = atom;

    size_t counter = 0;
    for (std::pair<int, int> &p : variable_source.get_table()) {
        if (p.first == int(j)) {
            p.second = p.second - argument_shift[counter];
        }
        counter++;
    }
}

void RuleBase::set_conditions(std::vector<DatalogAtom> new_rule_conditions) {
    conditions = new_rule_conditions;
}

void RuleBase::replace_single_condition(size_t j, DatalogAtom atom)  {
    conditions[j] = atom;
}

}
