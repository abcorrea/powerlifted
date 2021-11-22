#include "ff_heuristic.h"

#include "utils.h"

#include "../datalog/datalog.h"

#include "../datalog/annotations/annotation.h"


using namespace std;

class FFAnnotation : public datalog::Annotation {
    int rule_schema;
    std::vector<GroundAction> &pi_ff;

public:
    FFAnnotation(int rule_schema, std::vector<GroundAction> &pi_ff) : rule_schema(rule_schema),
                                                                      pi_ff(pi_ff) {}

    void operator()(datalog::GroundRule gr) override {
        std::vector<int> parameters = gr.get_parameters();
        GroundAction a(make_pair(rule_schema, parameters));
        pi_ff.push_back(a);
    }

    bool operator==(const Annotation &other) override {
        const FFAnnotation* other_ff = dynamic_cast<const FFAnnotation*>(&other);
        if (!other_ff) return false;
        return ((this->rule_schema != other_ff->rule_schema) and (&pi_ff == &other_ff->pi_ff));
    }
};



FFHeuristic::FFHeuristic(const Task &task) : datalog(std::move(initialize_datalog(task, get_annotation_generator()))),
                                             grounder(datalog, datalog::H_ADD) {}

int FFHeuristic::compute_heuristic(const DBState &s, const Task &task) {
    if (task.is_goal((s))) return 0;

    std::vector<datalog::Fact> state_facts = get_datalog_facts_from_state(s, task);

    int h = grounder.ground(datalog, state_facts, datalog.get_goal_atom_idx());

    datalog.reset_facts();
    for (const auto &r : datalog.get_rules())
        r->clean_up();
    if (h == std::numeric_limits<int>::max())
        return UNSOLVABLE_STATE;

    return h;

}

datalog::AnnotationGenerator FFHeuristic::get_annotation_generator() {
    datalog::AnnotationGenerator annotation_generator = [&](int action_schema_id, const Task &task) -> unique_ptr<datalog::Annotation> {
        // TODO Replace this check with enum
        if (action_schema_id < 0)
            return nullptr;
        return make_unique<FFAnnotation>(action_schema_id, pi_ff);
    };
    return annotation_generator;
}

