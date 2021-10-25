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

}