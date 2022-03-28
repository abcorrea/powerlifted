#ifndef SEARCH_DATALOG_TRANSFORMATIONS_GOAL_RULE_H_
#define SEARCH_DATALOG_TRANSFORMATIONS_GOAL_RULE_H_

#include "../datalog.h"

#include <vector>

namespace datalog {

void Datalog::add_goal_rule(const Task &task, AnnotationGenerator &annotation_generator) {
    std::string goal_predicate = "@goal-reachable";
    int idx = get_next_auxiliary_predicate_idx();
    map_new_predicates_to_idx.emplace(goal_predicate, idx);
    predicate_names.push_back(goal_predicate);
    DatalogAtom goal(Arguments(), idx, true);
    std::unique_ptr<Annotation> ann = annotation_generator(-1, task);

    goal_atom_idx = idx;

    std::vector<DatalogAtom> body;
    for (const AtomicGoal &ag: task.get_goal().goal) {
        std::vector<std::pair<int, int>> terms;
        for (int arg: ag.get_arguments()) {
            terms.emplace_back(arg, OBJECT); // All goal conditions are ground.
        }
        DatalogAtom atom(Arguments(terms), ag.get_predicate_index(), false);
        body.push_back(atom);
    }

    for (int nullary_goal_idx: task.get_goal().positive_nullary_goals) {
        body.emplace_back(Arguments(), nullary_goal_idx, false);
    }

    rules.emplace_back(std::make_unique<ProductRule>(0, goal, body, std::move(ann)));
}

}

#endif //SEARCH_DATALOG_TRANSFORMATIONS_GOAL_RULE_H_
