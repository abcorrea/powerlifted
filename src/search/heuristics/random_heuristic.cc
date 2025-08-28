#include "random_heuristic.h"

#include "utils.h"

#include "../datalog/datalog.h"

#include "../datalog/annotations/annotation.h"
#include "../datalog/datalog_to_task.h"
#include "../dump_task_to_PDDL.h"
using namespace std;

RandomHeuristic:: RandomHeuristic(const Task &task, DatalogTransformationOptions opts)   :  datalog(initialize_datalog(task, get_annotation_generator(), opts)),
    grounder(datalog, datalog::H_ADD) {

    Datalog_to_task dltt(datalog);


    
    Task new_task = dltt.getTask();

    // dumpToPDDLDomain(task, "domain.pddl");
    // dumpToPDDLProblem(task.get_initial_state(), task, "problem.pddl");
       


    }



datalog::AnnotationGenerator  RandomHeuristic::get_annotation_generator() {
    return [](int action_schema_id, const Task &task) -> unique_ptr<datalog::Annotation> {
        // TODO Replace this check with enum
        return nullptr;
    };

}
int RandomHeuristic::compute_heuristic(const DBState &s, const Task &task) {
    return 1;


    };


