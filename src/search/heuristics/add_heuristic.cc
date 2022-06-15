#include "add_heuristic.h"

#include "utils.h"

using namespace std;

AdditiveHeuristic::AdditiveHeuristic(const Task &task, DatalogTransformationOptions opts) :
    datalog(initialize_datalog(task, get_annotation_generator(), opts)),
    grounder(datalog, datalog::H_ADD) {}

datalog::AnnotationGenerator AdditiveHeuristic::get_annotation_generator() {
    return [&](int action_schema_id, const Task &task) -> unique_ptr<datalog::Annotation> {
        // TODO Replace this check with enum
        return nullptr;
    };
}

int AdditiveHeuristic::compute_heuristic(const DBState &s, const Task &task) {
    if (task.is_goal((s))) return 0;

    std::vector<datalog::Fact> state_facts = get_datalog_facts_from_state(s, task);

    int h = grounder.ground(datalog, state_facts, datalog.get_goal_atom_idx());
    //grounder.print_statistics(datalog);
    datalog.reset_facts();

    for (const auto &r : datalog.get_rules())
        r->clean_up();
    if (h == std::numeric_limits<int>::max())
        return UNSOLVABLE_STATE;

    useful_atoms = datalog.get_useful_atoms();

    return h;

}
