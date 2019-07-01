#include <fstream>
#include <iostream>
#include <vector>

#include "parser.h"
#include "goal_condition.h"

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
    for (int i = 0; i < initial_state_size; ++i) {
        string name;
        int index;
        int predicate_index;
        bool negated;
        int number_args;
        cin >> name >> index >> predicate_index >> negated >> number_args;
        vector<int> args;
        for (int j = 0; j < number_args; ++j) {
            int arg;
            cin >> arg;
            args.push_back(arg);
        }
        task.initial_state.addTuple(predicate_index, args);
    }

    // Read Goal State
    int goal_size;
    cin >> canary >> goal_size;
    if (canary != "GOAL") {
        cerr << "Error while reading goal description section." << endl;
        return false;
    }
    vector<AtomicGoal> goals;
    for (int i = 0; i < goal_size; ++i) {
        string name;
        int predicate_index;
        bool negated;
        int number_args;
        cin >> name >> predicate_index >> negated >> number_args;
        vector<int> args;
        for (int j = 0; j < number_args; ++j) {
            int arg;
            cin >> arg;
            args.push_back(arg);
        }
        goals.emplace_back(predicate_index, args, negated);
    }

    task.initializeGoal(goals);

    return true;

}