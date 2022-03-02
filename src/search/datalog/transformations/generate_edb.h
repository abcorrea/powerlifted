#ifndef SEARCH_DATALOG_TRANSFORMATIONS_GENERATE_EDB_H_
#define SEARCH_DATALOG_TRANSFORMATIONS_GENERATE_EDB_H_

#include "../datalog.h"

#include <stack>
#include <vector>

namespace datalog {

void Datalog::set_permanent_edb(StaticInformation static_information) {
    // What do we do with static information that is nullary?
    for (const auto &r: static_information.get_relations()) {
        for (const auto &tuple: r.tuples) {
            std::vector<std::pair<int, int>> args;
            for (int i: tuple) {
                args.emplace_back(i, OBJECT);
            }
            permanent_edb.emplace_back(Arguments(args), r.predicate_symbol, false);
        }
    }
    get_always_reachable_rule_heads();
}

void Datalog::get_always_reachable_rule_heads() {
    std::stack<size_t> to_be_deleted;
    for (size_t i = 0; i < rules.size(); ++i) {
        if (rules[i]->get_conditions().size()==0) {
            DatalogAtom eff = rules[i]->get_effect();
            to_be_deleted.push(i);
            permanent_edb.emplace_back(Fact(eff.get_arguments(),
                                            eff.get_predicate_index(),
                                            rules[i]->get_weight(),
                                            eff.is_pred_symbol_new()));
        }
    }
    while (!to_be_deleted.empty()) {
        size_t n = to_be_deleted.top();
        to_be_deleted.pop();
        rules.erase(rules.begin() + n);
    }
}

void Datalog::output_permanent_edb() {
    for (const Fact &f: permanent_edb) {
        output_atom(f);
        std::cout << " [cost: " << f.get_cost() << "]." << std::endl;
    }
}

}

#endif //SEARCH_DATALOG_TRANSFORMATIONS_GENERATE_EDB_H_
