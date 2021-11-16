#include "ff_heuristic.h"

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



FFHeuristic::FFHeuristic(const Task &task) {
    datalog::AnnotationGenerator annotation_generator = [&](int action_schema_id, const Task &task) -> unique_ptr<datalog::Annotation> {
        // TODO Replace this check with enum
        if (action_schema_id < 0)
            return nullptr;
        return make_unique<FFAnnotation>(action_schema_id, pi_ff);
    };
    datalog::Datalog dl(task, annotation_generator);

    std::cout << "@@@ ORIGINAL RULES: " << std::endl;
    dl.output_rules();

    cout << endl << "### ACTION PREDICATES REMOVED: " << endl;
    dl.remove_action_predicates(annotation_generator, task);
    dl.output_rules();

    cout << endl << "### CONVERT TO NORMAL FORM: " << endl;
    dl.convert_rules_to_normal_form(task);
    dl.output_rules();

    cout << endl << "### INTRODUCE GOAL RULE: " << endl;
    dl.add_goal_rule(task, annotation_generator);
    dl.output_rules();

    cout << "### PERMANENT EDB: " << endl;
    dl.set_permanent_edb(task.get_static_info());
    dl.output_permanent_edb();

}
