#include "rff_heuristic.h"

#include "utils.h"

#include "../datalog/datalog.h"

#include "../datalog/annotations/annotation.h"

using namespace std;

class RFFAnnotation : public datalog::Annotation {
    int cost;
    int &total_cost;

public:
    RFFAnnotation(int cost, int &total_cost) : cost(cost), total_cost(total_cost) {}

    void execute(int head,
                 const datalog::Datalog &datalog) override {
        total_cost += cost;
    }

    bool operator==(const Annotation &other) override {
        const RFFAnnotation* other_rff = dynamic_cast<const RFFAnnotation*>(&other);
        if (!other_rff) return false;
        return ((this->cost != other_rff->cost) and (&this->total_cost == &other_rff->total_cost));
    }
};


RFFHeuristic::RFFHeuristic(const Task &task, DatalogTransformationOptions opts) :
    datalog(initialize_datalog(task, get_annotation_generator(), opts)),
    grounder(datalog, datalog::H_ADD) {}

int RFFHeuristic::compute_heuristic(const DBState &s, const Task &task) {
    if (task.is_goal((s))) return 0;

    rff_cost = 0; // resets rff_cost

    std::vector<datalog::Fact> state_facts = get_datalog_facts_from_state(s, task);

    int h_add = grounder.ground(datalog, state_facts, datalog.get_goal_atom_idx());

    datalog.reset_facts();
    for (const auto &r : datalog.get_rules())
        r->clean_up();
    if (h_add == std::numeric_limits<int>::max())
        return UNSOLVABLE_STATE;

    useful_atoms = datalog.get_useful_atoms();

    return rff_cost;

}

datalog::AnnotationGenerator RFFHeuristic::get_annotation_generator() {
    datalog::AnnotationGenerator annotation_generator = [&](int action_schema_id, const Task &task) -> unique_ptr<datalog::Annotation> {
        if (action_schema_id < 0)
            return nullptr;
        int cost = task.get_action_schema_by_index(action_schema_id).get_cost();
        return make_unique<RFFAnnotation>(cost, rff_cost);
    };
    return annotation_generator;
}

