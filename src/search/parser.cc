#include <fstream>
#include <iostream>
#include <vector>
#include <boost/algorithm/string.hpp>

#include "parser.h"
#include "goal_condition.h"
#include "action_schema.h"

using namespace std;

bool parse(Task &task, const ifstream &in) {
    string repr;
    cin >> repr;
    if (repr != "SPARSE-REPRESENTATION") {
        cerr << "Representation is not sparse. Not supported." << endl;
        return false;
    }

    string canary; // string used to guarantee consistency throughout the parsing

    int number_types;
    cin >> canary >> number_types;
    if (canary != "TYPES") {
        cerr << "Error while reading types section." << endl;
        return false;
    }
    for (int i = 0; i < number_types; ++i) {
        string type_name;
        int type_index;
        cin >> type_name >> type_index;
        task.addType(type_name);
    }

    int number_predicates;
    cin >> canary >> number_predicates;
    if (canary != "PREDICATES") {
        cerr << "Error while reading predicate section." << endl;
        return false;
    }
    for (int j = 0; j < number_predicates; ++j) {
        string predicate_name;
        int index;
        int number_args;
        bool static_pred;
        cin >> predicate_name >> index >> number_args >> static_pred;
        vector<int> types;
        if (number_args == 0) {
            task.nullary_predicates.insert(index);
        }
        for (int i = 0; i < number_args; ++i) {
            int type;
            cin >> type;
            types.push_back(type);
        }
        task.addPredicate(predicate_name, index, number_args, static_pred, types);
    }

    // Read Objects
    int number_objects;
    cin >> canary >> number_objects;
    if (canary != "OBJECTS") {
        cerr << "Error while reading object section." << endl;
        return false;
    }
    for (int i = 0; i < number_objects; ++i) {
        string name;
        int index;
        int n;
        cin >> name >> index >> n;
        vector<int> types;
        for (int j = 0; j < n; ++j) {
            int t;
            cin >> t;
            types.push_back(t);
        }
        task.addObject(name, index, types);
    }

    // Read Initial State
    task.initializeEmptyInitialState();
    int initial_state_size;
    cin >> canary >> initial_state_size;
    if (canary != "INITIAL-STATE") {
        cerr << "Error while reading initial state section." << endl;
        return false;
    }
    task.initial_state.nullary_atoms.clear();
    task.initial_state.nullary_atoms.resize(task.predicates.size(), false);
    for (int i = 0; i < initial_state_size; ++i) {
        string name;
        int index;
        int predicate_index;
        bool negated;
        int number_args;
        cin >> name >> index >> predicate_index >> negated >> number_args;
        vector<int> args;
        if (number_args == 0) {
            assert(task.nullary_predicates.find(predicate_index) != task.nullary_predicates.end());
            task.initial_state.nullary_atoms[predicate_index] = true;
        }
        for (int j = 0; j < number_args; ++j) {
            int arg;
            cin >> arg;
            args.push_back(arg);
        }
        if (!task.initial_state.nullary_atoms[predicate_index]) {
            if (!task.predicates[predicate_index].isStaticPredicate())
                task.initial_state.addTuple(predicate_index, args);
            else
                task.static_info.addTuple(predicate_index, args);
        }
    }

    // Read Goal State
    int goal_size;
    cin >> canary >> goal_size;
    if (canary != "GOAL") {
        cerr << "Error while reading goal description section." << endl;
        return false;
    }
    vector<AtomicGoal> goals;
    unordered_set<int> positive_nullary_goals, negative_nullary_goals;
    for (int i = 0; i < goal_size; ++i) {
        string name;
        int predicate_index;
        bool negated;
        int number_args;
        cin >> name >> predicate_index >> negated >> number_args;
        if (number_args == 0) {
            if (negated)
                negative_nullary_goals.insert(predicate_index);
            else
                positive_nullary_goals.insert(predicate_index);
            continue;
        }
        vector<int> args;
        for (int j = 0; j < number_args; ++j) {
            int arg;
            cin >> arg;
            args.push_back(arg);
        }
        goals.emplace_back(predicate_index, args, negated);
    }
    task.initializeGoal(goals, positive_nullary_goals, negative_nullary_goals);

    // Read Action Schemas
    int number_action_schemas;
    cin >> canary >> number_action_schemas;
    if (canary != "ACTION-SCHEMAS") {
        cerr << "Error while reading action schemas section." << endl;
        return false;
    }
    vector<ActionSchema> actions;
    for (int i = 0; i < number_action_schemas; ++i) {
        string name;
        int cost, args, precond_size, eff_size;
        cin >> name >> cost >> args >> precond_size >> eff_size;
        vector<Parameter> parameters;
        vector<Atom> preconditions, effects;
        vector<pair<int,int>> inequalities;
        vector<bool> positive_nul_precond(task.predicates.size(), false),
                negative_nul_precond(task.predicates.size(), false),
                positive_nul_eff(task.predicates.size(), false),
                negative_nul_eff(task.predicates.size(), false);
        for (int j = 0; j < args; ++j) {
            string param_name;
            int index, type;
            cin >> param_name >> index >> type;
            parameters.emplace_back(param_name, index, type);
        }
        for (int j = 0; j <  precond_size; ++j) {
            string precond_name;
            int index;
            bool negated;
            int arguments_size;
            cin >> precond_name >> index >> negated >> arguments_size;
            if (arguments_size == 0) {
                assert(task.nullary_predicates.find(index) != task.nullary_predicates.end());
                if (!negated)
                    positive_nul_precond[index] = true;
                else
                    negative_nul_precond[index] = true;
                continue;
            }
            bool is_inequality = (boost::iequals(precond_name, "=") and negated);
            vector<Argument> arguments;
            if (is_inequality) {
                assert (arguments_size == 2);
                int obj1, obj2;
                char c, d;
                cin >> c >> obj1 >> d >> obj2;
                arguments.emplace_back(obj1, c == 'c');
                arguments.emplace_back(obj2, d == 'c');
                if (find(inequalities.begin(), inequalities.end(), make_pair(obj2, obj1))
                    != inequalities.end()) {
                    continue;
                }
                inequalities.emplace_back(obj1, obj2);
            }
            else {
                for (int k = 0; k < arguments_size; ++k) {
                    char c;
                    int obj_index;
                    cin >> c >> obj_index;
                    if (c == 'c') {
                        arguments.emplace_back(obj_index, true);
                    } else if (c == 'p') {
                        arguments.emplace_back(obj_index, false);
                    } else {
                        cerr << "Error while reading action schema " << name << ". Argument is neither constant or "
                                                                                "object" << endl;
                        exit(-1);
                    }
                }
            }
            preconditions.emplace_back(move(precond_name), index, move(arguments), negated);
        }
        for (int j = 0; j <  eff_size; ++j) {
            string eff_name;
            int index;
            bool negated;
            int arguments_size;
            cin >> eff_name >> index >> negated >> arguments_size;
            vector<Argument> arguments;
            if (arguments_size == 0) {
                assert(task.nullary_predicates.find(index) != task.nullary_predicates.end());
                if (!negated)
                    positive_nul_eff[index] = true;
                else
                    negative_nul_eff[index] = true;
                continue;
            }
            for (int k = 0; k < arguments_size; ++k) {
                char c;
                int obj_index;
                cin >> c >> obj_index;
                if (c == 'c') {
                    arguments.emplace_back(obj_index, true);
                }
                else if (c == 'p') {
                    arguments.emplace_back(obj_index, false);
                }
                else {
                    cerr << "Error while reading action schema " << name << ". Argument is neither constant or "
                                                                            "object"<< endl;
                    exit(-1);
                }
            }
            effects.emplace_back(move(eff_name), index, move(arguments), negated);
        }
        ActionSchema a(name, i, cost, parameters, preconditions, effects, inequalities,
                       positive_nul_precond, negative_nul_precond, positive_nul_eff, negative_nul_eff);
        actions.push_back(a);
    }
    task.initializeActionSchemas(actions);

    return true;

}