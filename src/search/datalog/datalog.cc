#include "datalog.h"

#include <iostream>

using namespace datalog;
using namespace std;

Datalog::Datalog(const Task &task) : task(task) {
    for (const ActionSchema &schema : task.get_action_schemas()) {
        for (const Atom &eff : schema.get_effects()) {
            cout << eff.get_name();
           print_parameters(eff.get_arguments());
            cout << " :- ";
            size_t number_conditions = schema.get_precondition().size();
            for (const Atom &condition :schema.get_precondition()) {
                number_conditions--;
                if (condition.is_negated())
                    continue;
                cout << condition.get_name();
                print_parameters(condition.get_arguments());
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


void Datalog::print_parameters(std::vector<Argument> v) {
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
