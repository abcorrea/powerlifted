#include "parser.h"

#include "rules/join.h"
#include "rules/product.h"
#include "rules/project.h"

#include <memory>
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>

using namespace std;

namespace lifted_heuristic {

int number_of_atoms = 0;
int number_of_facts = 0;
int number_of_rules = 0;
int number_of_objects = 0;

LogicProgram parse_logic_program(ifstream &in) {
    cout << "Parsing file..." << endl;

    unordered_map<string, int> map_object_to_index;
    unordered_map<string, int> map_atom_to_index;
    unordered_map<int, string> map_index_to_atom;

    vector<Object> lp_objects;
    vector<Fact> lp_facts;
    vector<unique_ptr<RuleBase>> rules;

    string line;

    while (getline(in, line)) {
        // Check if it is a fact or a rule
        if (is_warning_message(line)) {
            // One of the translator warning message, skip!
            continue;
        }
        if (line.find(":-")!=string::npos) {
            // Rule
            int number_of_vars_current_rule = 0; // Variables have negative counter
            int weight = 0;
            string head = line.substr(0, line.find(':'));
            string body = line.substr(line.find(':'), line.find('[')); // Still contains ':-'
            string weight_function = line.substr(line.find('['), line.find(']'));

            weight = process_weight(weight_function);

            string rule_type = head.substr(0, head.find(' '));
            string head_atom_name_and_args = head.substr(head.find(' '));
            string head_predicate = get_atom_name(head_atom_name_and_args);
            vector<string>
                head_arguments =
                extract_arguments_from_atom(head_atom_name_and_args);

            unordered_map<string, int> map_variables_to_index;
            Arguments head_argument_indices = transform_args_into_indices(
                map_object_to_index,
                map_variables_to_index,
                head_arguments,
                lp_objects,
                number_of_vars_current_rule);

            auto head_pred_pair =
                map_atom_to_index.try_emplace(head_predicate, number_of_atoms);
            if (head_pred_pair.second) {
                map_index_to_atom.emplace(number_of_atoms, head_predicate);
                number_of_atoms++;
            }
            Atom head_atom(head_argument_indices,
                           map_atom_to_index[head_predicate]);

            vector<string> condition_atoms_strings = get_rule_conditions(body);
            vector<Atom> condition_atoms;
            condition_atoms.reserve(condition_atoms_strings.size());
            for (const auto& s : condition_atoms_strings) {
                string atom_name = get_atom_name(s);
                auto atom_pair =
                    map_atom_to_index.try_emplace(atom_name, number_of_atoms);
                if (atom_pair.second) {
                    map_index_to_atom.emplace(number_of_atoms, atom_name);
                    number_of_atoms++;
                }
                vector<string> atom_arguments = extract_arguments_from_atom(s);
                Arguments indices;
                if (!atom_arguments.empty()) {
                    indices = transform_args_into_indices(
                        map_object_to_index,
                        map_variables_to_index,
                        atom_arguments,
                        lp_objects,
                        number_of_vars_current_rule);
                }
                condition_atoms.emplace_back(indices,
                                             map_atom_to_index[atom_name]);
            }

            if (boost::iequals(rule_type, "project")) {
                // Project rule
                rules.emplace_back(make_unique<ProjectRule>(weight, head_atom, condition_atoms));
            } else if (boost::iequals(rule_type, "join")) {
                // Join rule
                rules.emplace_back(make_unique<JoinRule>(weight, head_atom, condition_atoms));
            } else if (boost::iequals(rule_type, "product")) {
                // Product rule
                rules.emplace_back(make_unique<ProductRule>(weight, head_atom, condition_atoms));
            }

            number_of_rules++;
        } else {
            // Fact
            string weight_function = line.substr(line.find('['), line.find(']'));

            int weight = process_weight(weight_function);

            string predicate = get_atom_name(line);
            auto pred_pair =
                map_atom_to_index.try_emplace(predicate, number_of_atoms);
            if (pred_pair.second) {
                map_index_to_atom.emplace(number_of_atoms, predicate);
                number_of_atoms++;
            }

            vector<string> arguments = extract_arguments_from_atom(line);

            Arguments arguments_indices;
            for (const auto &argument : arguments) {
                auto it_pair =
                    map_object_to_index.try_emplace(argument, number_of_objects);
                if (it_pair.second) {
                    // Object is new to the map. Increase counter.
                    lp_objects.emplace_back(argument);
                    number_of_objects++;
                }
                arguments_indices.push_back(map_object_to_index[argument], OBJECT);
            }

            lp_facts.emplace_back(arguments_indices,
                                  map_atom_to_index[predicate], weight);
            number_of_facts++;
        }
    }

    // Loop over the facts setting their fact indices
    for (Fact &f : lp_facts)
        f.set_fact_index();

    return LogicProgram(move(lp_facts),
        move(lp_objects),
        move(rules),
        move(map_index_to_atom),
        move(map_atom_to_index),
        move(map_object_to_index));
}

bool is_warning_message(const string &line) {
    if (line.find("Warning:")!=string::npos) {
        return true;
    }
    if (line.find("Duplicate arguments:")!=string::npos) {
        return true;
    }
    if (line.find("Trivial rules:")!=string::npos) {
        return true;
    }
    return false;
}

/*
 * Get atom name
 */
string get_atom_name(const string &str) {
    string new_str = str.substr(0, str.find('('));
    boost::erase_all(new_str, ",");
    return boost::trim_copy(new_str);
}

/*
 * Extract the arguments (i.e., parameters) of atoms. The atom can be anything
 * in the format X(a, b, c, ..., z) where X is a string and a, b, c, ..., z are
 * also strings separated by comma.
 */
vector<string> extract_arguments_from_atom(const string &atom) {
    unsigned args_start = atom.find('(');
    unsigned args_end = atom.find(')');
    string arguments_in_str = boost::trim_copy(atom.substr(args_start, args_end - args_start));
    if (args_start==args_end - 1) {
        // Nullary atoms
        return vector<string>(0);
    }
    arguments_in_str.erase(0, 1); // Removes first delimiter, '('
    if (arguments_in_str.back()=='.')
        arguments_in_str.pop_back(); // Remove last delimiter, '.'
    if (arguments_in_str.back()==')')
        arguments_in_str.pop_back(); // Remove last delimiter, ')'
    boost::erase_all(arguments_in_str, " "); // Remove white spaces

    vector<string> arguments;
    if (!arguments_in_str.empty())
        boost::split(arguments, arguments_in_str, boost::is_any_of(","));
    else
        arguments.resize(0);
    return arguments;
}

/*
 * Transforms a vector of arguments (having constants and variables) into a vector
 * of integers. Free variables have negative value.
 */
Arguments transform_args_into_indices(
    unordered_map<string, int> &map_objects,
    unordered_map<string, int> &map_variables,
    const vector<string> &arguments,
    vector<Object> &lp_objects,
    int &number_of_vars_current_rule) {
    vector<Term> indices(arguments.size());
    int counter = 0;
    for (const auto &a : arguments) {
        if (a.front()=='?') {
            // Variable
            auto it_pair =
                map_variables.try_emplace(a, number_of_vars_current_rule);
            if (it_pair.second) {
                // Variable is not new to the map. Increase counter.
                number_of_vars_current_rule++;
            }
            indices[counter++] = Term(map_variables[a], VARIABLE);
        } else {
            if (a.empty())
                continue;
            auto it_pair = map_objects.try_emplace(a, number_of_objects);
            if (it_pair.second) {
                // Object is new to the map. Increase counter.
                lp_objects.emplace_back(a);
                number_of_objects++;
            }
            indices[counter++] = Term(map_objects[a], OBJECT);
        }
    }
    return Arguments(move(indices));
}

vector<string> get_rule_conditions(string &body) {
    vector<string> condition_atoms;
    boost::erase_all(body, ":- ");
    boost::split(condition_atoms, body, boost::is_any_of(")"));

    condition_atoms.pop_back(); // Last one is always a dangling '.'

    return condition_atoms;
}

int process_weight(string weight_function) {
    // TODO Implement parsing for lifted cost function, e.g. (road-length ?l1 ?l2)

    // case for constant cost
    weight_function.erase(0, 1);
    weight_function.erase(weight_function.size() - 1);
    return stoi(weight_function);
}

}