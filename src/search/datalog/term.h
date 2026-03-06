#ifndef GROUNDER_TERM_H
#define GROUNDER_TERM_H

#include <cstddef>
#include <functional>

namespace datalog {

enum TERM_TYPES { OBJECT, VARIABLE };

class Term {
    int index : 31;
    int type : 1;

public:
    Term() = default;

    Term(int i, int t) : index(i), type(t) {}

    int get_index() const { return index; }

    bool is_object() const { return (type == OBJECT); }

    void set_term_to_object(int j)
    {
        index = j;
        type = OBJECT;
    }

    friend bool operator==(const Term &lhs, const Term &rhs)
    {
        return ((lhs.get_index() == rhs.get_index()) and (lhs.is_object() == rhs.is_object()));
    }

    friend bool operator!=(const Term &lhs, const Term &rhs) { return (!(lhs == rhs)); }
};

}  // namespace datalog

template <>
struct std::hash<datalog::Term> {
    std::size_t operator()(const datalog::Term &t) const
    {
        std::size_t seed = std::hash<int>{}(t.get_index());
        // 0x9e3779b9 = 2^32 / phi (golden ratio); see Knuth TAOCP Vol. 3, Sec. 6.4
        seed ^= std::hash<bool>{}(t.is_object()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

#endif  // GROUNDER_TERM_H
