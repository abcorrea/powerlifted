#include "utils.h"

datalog::Datalog initialize_datalog(const Task &task, datalog::AnnotationGenerator annotation_generator) {
    datalog::Datalog dl(task, annotation_generator);

    //std::cout << "@@@ ORIGINAL RULES: " << std::endl;
    //dl.output_rules();

    //std::cout << std::endl << "### ACTION PREDICATES REMOVED: " << std::endl;
    dl.remove_action_predicates(annotation_generator, task);
    //dl.output_rules();
    //exit(0);
    //std::cout << std::endl << "### CONVERT TO NORMAL FORM: " << std::endl;
    dl.convert_rules_to_normal_form(task);
    //dl.output_rules();
    //exit(0);

    //std::cout << std::endl << "### INTRODUCE GOAL RULE: " << std::endl;
    dl.add_goal_rule(task, annotation_generator);
    //dl.output_rules();

    //std::cout << std::endl << "### RENAME VARIABLES: " << std::endl;
    dl.rename_variables();
    //dl.output_rules();


    //std::cout << std::endl << "### REMOVE EQUIVALENT RULES: " << std::endl;
    while (dl.remove_duplicate_rules());
    //dl.output_rules();

    //std::cout << "### PERMANENT EDB: " << std::endl;
    dl.set_permanent_edb(task.get_static_info());
    //dl.output_permanent_edb();

    dl.update_rule_indices();

    dl.print_statistics();

    //std::cout << std::endl << "@@@ FINAL RULES: " << std::endl;
    //dl.output_rules();
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
