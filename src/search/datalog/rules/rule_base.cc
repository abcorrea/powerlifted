#include "rule_base.h"

#include <algorithm>

using namespace std;

namespace datalog {

int RuleBase::next_index = 0;


void RuleBase::output_variable_table() {
    int v = 0;
    for (const std::pair<int, int> p : variable_source) {
        bool is_found_in_rule = (p.first < 0);
        int position_in_body = p.second;
        int body_position = get_position_of_atom_in_same_body_rule(p.first);
        if (is_found_in_rule) {
            std::cout << v << ' ' << "BODY " << body_position << ' ' << position_in_body << std::endl;
        }
        else {
            std::cout << v << ' ' << "AUX_BODY " << body_position << ' ' << position_in_body << std::endl;
        }
        ++v;
    }
}

void RuleBase::update_conditions(DatalogAtom new_atom, const std::vector<DatalogAtom> &new_rule_conditions, std::vector<size_t> body_ids) {
    /*
     *
     * TODO Implement class to keep track of variable source table and include map from terms
     * to table entries so we can do this update correctly and more elegantly.
     *
     */
    std::sort(body_ids.begin(), body_ids.end());
    std::vector<DatalogAtom> old_conditions = conditions;
    for (int i = int(body_ids.size()) -1; i >= 0; --i) {
        conditions.erase(conditions.begin() + body_ids[i]);
    }
    conditions.push_back(new_atom);

    int final_index = conditions.size() - 1;

    for (std::pair<int, int> &p : variable_source) {
        int canonical_index = p.first;
        if (canonical_index < 0)
            canonical_index = (canonical_index*-1)-1; // Conversion to positive index

        if (std::find(body_ids.begin(), body_ids.end(), canonical_index) != body_ids.end()) {
            // First, we find out which term is this entry p referring to
            Term t = old_conditions[canonical_index].argument(p.second);


            p.first = final_index; // Now this points to the final conditions of the rule

            // Then, we find out the position of its entry in the table
            bool found_correspondence = false;
            for (const DatalogAtom &b : new_rule_conditions) {
                int position_counter = 0;
                for (const Term &body_arg : b.get_arguments()) {
                    if (body_arg.is_object()) continue;
                    if (body_arg.get_index() == t.get_index()) {
                        p.second = position_counter;
                        found_correspondence = true;
                        break;
                    }
                    position_counter++;
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
                    if (p.first < int(body_ids[i]))
                        p.first = p.first + 1;
                }
                else {
                    if (p.first > int(body_ids[i]))
                        p.first = p.first - 1;
                }
            }
        }
    }

}

}
