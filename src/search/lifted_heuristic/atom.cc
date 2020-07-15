#include "atom.h"

using namespace std;

int Atom::next_index = 0;

void Atom::print_atom(const vector<Object> &obj,
                      const unordered_map<int, string> &map_index_to_atom) const {
    cout << map_index_to_atom.at(predicate_index) << '(';
    size_t cont = 0;
    for (const Term &t : arguments) {
        if (t.is_object()) {
            // a >= 0 --> object. Print its name
            cout << obj[t.get_index()].get_name();
        } else {
            // a < 0 --> free variable. Print the free variable of the rule.
            cout << '?' << char('A' + t.get_index());
        }
        cont++;
        if (cont!=arguments.size())
            cout << ", ";
    }
    cout << ')' << endl;
}
