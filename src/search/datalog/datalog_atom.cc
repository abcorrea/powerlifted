#include "datalog_atom.h"

namespace  datalog {

using namespace std;

int DatalogAtom::next_index = 0;

DatalogAtom::DatalogAtom(const Atom &atom) {
    index = next_index++;
    new_pred_symbol = false;
    predicate_index = atom.get_predicate_symbol_idx();
    vector<pair<int,int>> args;
    for (Argument a : atom.get_arguments()) {
        if (a.is_constant()) {
            args.emplace_back(a.get_index(), datalog::OBJECT);
        }
        else {
            args.emplace_back(a.get_index(), datalog::VARIABLE);
        }
    }
    arguments = Arguments(args);
}

DatalogAtom::DatalogAtom(const ActionSchema &schema, int idx) {
    index = next_index++;
    new_pred_symbol = true;
    predicate_index = idx;
    vector<pair<int,int>> args;
    for (Parameter a : schema.get_parameters()) {
        args.emplace_back(a.get_index(), datalog::VARIABLE);
    }
    arguments = Arguments(args);
}

bool DatalogAtom::is_nullary() const {
    return (arguments.size() == 0);
}

bool DatalogAtom::is_ground()  const {
    for (const Term term : arguments) {
        if (!term.is_object()) {
            return false;
        }
    }
    return true;
}

bool DatalogAtom::share_variables(const DatalogAtom &atom) const {
    for (const Term &term : arguments)
        if (!term.is_object())
            for (const Term &term2 : atom.arguments)
                if (term == term2)
                    return true;
    return false;
}

}