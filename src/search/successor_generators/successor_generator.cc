#include "successor_generator.h"

#include "../action.h"
#include "../action_schema.h"
#include "../states/state.h"
#include "../task.h"
#include "../database/table.h"
#include "generic_join_successor.h"

#include <cassert>
#include <vector>

using namespace std;

SuccessorGenerator::SuccessorGenerator(const Task &task)  :
    static_information(task.get_static_info())
{
    obj_per_type.resize(task.type_names.size());
    for (const Object &obj : task.objects) {
        for (int type : obj.getTypes()) {
            obj_per_type[type].push_back(obj.getIndex());
        }
    }
    is_predicate_static.reserve(static_information.get_relations().size());
    for (const auto &r : static_information.get_relations()) {
        is_predicate_static.push_back(!r.tuples.empty());
    }
}


