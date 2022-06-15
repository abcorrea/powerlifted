#include "utils.h"

datalog::Datalog initialize_datalog(const Task &task,
                                    datalog::AnnotationGenerator annotation_generator,
                                    const DatalogTransformationOptions &opts) {
    datalog::Datalog dl(task, annotation_generator);

    if (opts.get_remove_action_predicates())
        dl.remove_action_predicates(annotation_generator, task);

    // These transformations are always done.
    dl.convert_rules_to_normal_form(task);
    dl.add_goal_rule(task, annotation_generator);

    if (opts.get_rename_vars())
        dl.rename_variables();

    if (opts.get_collapse_predicates())
        while (dl.remove_duplicate_rules());

    // These transformations are always done too.
    dl.set_permanent_edb(task.get_static_info());
    dl.update_rule_indices();

    dl.print_statistics();

    return dl;
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
