#include "utils.h"

datalog::Datalog initialize_datalog(const Task &task, datalog::AnnotationGenerator annotation_generator) {
    datalog::Datalog dl(task, annotation_generator);

    //std::cout << "@@@ ORIGINAL RULES: " << std::endl;
    //dl.output_rules();

    //cout << endl << "### ACTION PREDICATES REMOVED: " << endl;
    dl.remove_action_predicates(annotation_generator, task);
    //dl.output_rules();

    //cout << endl << "### CONVERT TO NORMAL FORM: " << endl;
    dl.convert_rules_to_normal_form(task);
    //dl.output_rules();

    //cout << endl << "### INTRODUCE GOAL RULE: " << endl;
    dl.add_goal_rule(task, annotation_generator);
    //dl.output_rules();

    //cout << "### PERMANENT EDB: " << endl;
    dl.set_permanent_edb(task.get_static_info());
    //dl.output_permanent_edb();

    dl.update_rule_indices();

    dl.print_statistics();

    return std::move(dl);
}

std::vector<datalog::Fact> get_datalog_facts_from_state(const DBState &s, const Task &task) {
    std::vector<datalog::Fact> facts;
    for (const auto &r: s.get_relations()) {
        for (const auto &tuple: r.tuples) {
            std::vector<std::pair<int, int>> args;
            for (int i: tuple) {
                args.emplace_back(i, datalog::OBJECT);
            }
            facts.emplace_back(datalog::Arguments(args), r.predicate_symbol, false);
        }
    }
    for (size_t i = 0; i < s.get_nullary_atoms().size(); ++i) {
        if (s.get_nullary_atoms()[i]) {
            facts.emplace_back(datalog::Arguments(), i, false);
        }
    }
    return facts;
}
