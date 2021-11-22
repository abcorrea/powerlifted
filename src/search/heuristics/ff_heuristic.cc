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



FFHeuristic::FFHeuristic(const Task &task) : datalog(std::move(initialize_datalog(task))),
                                             grounder(datalog, datalog::H_ADD) {}

int FFHeuristic::compute_heuristic(const DBState &s, const Task &task) {
    if (task.is_goal((s))) return 0;

    std::vector<datalog::Fact> state_facts;

    for (const auto &r: s.get_relations()) {
        for (const auto &tuple: r.tuples) {
            std::vector<std::pair<int, int>> args;
            for (int i: tuple) {
                args.emplace_back(i, datalog::OBJECT);
            }
            state_facts.emplace_back(datalog::Arguments(args), r.predicate_symbol, false);
        }
    }
    for (size_t i = 0; i < s.get_nullary_atoms().size(); ++i) {
        if (s.get_nullary_atoms()[i]) {
            state_facts.emplace_back(datalog::Arguments(), i, false);
        }
    }

    int h = grounder.ground(datalog, state_facts, datalog.get_goal_atom_idx());

    datalog.reset_facts();
    for (const auto &r : datalog.get_rules())
        r->clean_up();
    if (h == std::numeric_limits<int>::max())
        return UNSOLVABLE_STATE;

    return h;

}

datalog::Datalog FFHeuristic::initialize_datalog(const Task &task) {
    datalog::AnnotationGenerator annotation_generator = [&](int action_schema_id, const Task &task) -> unique_ptr<datalog::Annotation> {
        // TODO Replace this check with enum
        if (action_schema_id < 0)
            return nullptr;
        return make_unique<FFAnnotation>(action_schema_id, pi_ff);
    };
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

    return std::move(dl);
}

