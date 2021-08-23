#include "datalog.h"

#include <iostream>

using namespace datalog;
using namespace std;

Datalog::Datalog(const Task &task) : task(task) {
    for (const ActionSchema &schema : task.get_action_schemas()) {
        for (const Atom &eff : schema.get_effects()) {
            if (eff.is_negated())
                continue;
            print_atom(eff);
            cout << " :- ";
            size_t number_conditions = schema.get_precondition().size();
            for (const Atom &condition : schema.get_precondition()) {
                number_conditions--;
                if (condition.is_negated())
                    continue;
                print_atom(condition);
                if (number_conditions != 0)
                    cout << ", ";
                else
                    cout << "." << endl;
            }
        }
    }
}

void Datalog::dump_rules() {
    cout << "Rules: " << endl;
}

void Datalog::print_atom(const Atom &atom) {
    cout << atom.get_name();
    print_parameters(atom.get_arguments());
}

void Datalog::print_parameters(const vector<Argument>& v) {
    cout << '(';
    int number_params = v.size();
    for (auto arg : v) {
        if (arg.is_constant()) {
            cout << task.get_object_name(arg.get_index());
        } else {
            cout << "?v" << arg.get_index();
        }
        if (--number_params > 0) cout << ", ";
    }
    cout << ')';
}
