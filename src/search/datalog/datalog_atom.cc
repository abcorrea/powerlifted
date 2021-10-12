#include "datalog_atom.h"

namespace  datalog {

using namespace std;

int DatalogAtom::next_index = 0;

DatalogAtom::DatalogAtom(const Atom &atom) {
    index = next_index++;
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

}