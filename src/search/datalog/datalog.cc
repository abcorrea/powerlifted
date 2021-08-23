#include "datalog.h"

#include <iostream>

using namespace datalog;
using namespace std;

Datalog::Datalog(const Task &task) {
    for (const ActionSchema &schema : task.get_action_schemas()) {
        for (const Atom &eff : schema.get_effects()) {
            cout << eff.get_name() << "(";
              cout << ") :- ";
            size_t number_conditions = schema.get_precondition().size();
            for (const Atom &condition :schema.get_precondition()) {
                number_conditions--;
                if (condition.is_negated())
                    continue;
                cout << condition.get_name();
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
