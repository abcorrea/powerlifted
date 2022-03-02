#ifndef GROUNDER_TERM_H
#define GROUNDER_TERM_H

#include <boost/functional/hash.hpp>

namespace  datalog {

enum TERM_TYPES { OBJECT, VARIABLE };

class Term {
    int index : 31;
    int type : 1;

public:
    Term() = default;

    Term(int i, int t) : index(i), type(t) {}

    int get_index() const {
        return index;
    }

    bool is_object() const {
        return (type==OBJECT);
    }

    void set_term_to_object(int j) {
        index = j;
        type = OBJECT;
    }

    friend std::size_t hash_value(const Term &t) {
        std::size_t seed = 0;
        boost::hash_combine(seed, t.get_index());
        boost::hash_combine(seed, t.is_object());
        return seed;
    }

    friend bool operator==(const Term &lhs, const Term &rhs) {
        return ((lhs.get_index()==rhs.get_index()) and (lhs.is_object()==rhs.is_object()));
    }

    friend bool operator!=(const Term &lhs, const Term &rhs) {
        return (!(lhs==rhs));
    }

};

}

#endif //GROUNDER_TERM_H
