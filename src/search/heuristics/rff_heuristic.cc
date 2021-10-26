#include "rff_heuristic.h"

#include "../datalog/datalog.h"

#include "../datalog/annotations/annotation.h"

using namespace std;

class RFFAnnotation : public datalog::Annotation {
    int cost;
    int &total_cost;

public:
    RFFAnnotation(int cost, int &total_cost) : cost(cost), total_cost(total_cost) {}

    void operator()(datalog::GroundRule gr) override {
        total_cost += cost;
    }

    bool operator==(const Annotation &other) override {
        const RFFAnnotation* other_rff = dynamic_cast<const RFFAnnotation*>(&other);
        if (!other_rff) return false;
        return ((this->cost != other_rff->cost) and (&this->total_cost == &other_rff->total_cost));
    }
};



RFFHeuristic::RFFHeuristic(const Task &task) {
    datalog::AnnotationGenerator ann = [&](int action_schema_id, const Task &task) -> unique_ptr<datalog::Annotation> {
        if (action_schema_id < 0)
            return nullptr;
        int cost = task.get_action_schema_by_index(action_schema_id).get_cost();
        return make_unique<RFFAnnotation>(cost, rff_cost);
    };
    datalog::Datalog dl(task, ann);
}
