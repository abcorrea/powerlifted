#include <iostream>
#include <vector>
#include <fstream>

#include "task.h"

using namespace std;

bool parse(Task &task, const ifstream &in);

int main(int argc, char *argv[]) {
    cout << "Initializing planner" << endl;

    if (argc != 2) {
        cerr << "Usage: ./planner [TASK INPUT]" << endl;
        exit(-1);
    }

    // Remember to change it when it is not debugging anymore
    cout << argv[1] << endl;
    ifstream in(argv[1]);
    if (!in) {
        cerr << "Error opening the task file." << endl;
        exit(-1);
    }

    cout << "Reading task description file." << endl;
    cin.rdbuf(in.rdbuf());

    // Parse file
    string domain_name, task_name;
    cin >> domain_name >> task_name;
    Task task(domain_name, task_name);
    cout << task.getDomainName() << " " << task.getTaskName() << endl;

    bool parsed = parse(task, in);
    if (!parsed) {
        cerr << "Parser failed." << endl;
        return -1;
    }

    /*
     * TODO
     * We probably want to create some more refined data structures. For example, it would be nice to have an easy way
     * to get all objects of a given type. An unordered map/set might suffice for this example.  For other attributes,
     * this might also be interesting.  Maybe in the future we can do it while parsing.
     *
     */

    return 0;
}

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

    return true;

}