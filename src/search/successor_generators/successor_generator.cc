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
    is_predicate_static.reserve(static_information.get_relations().size());
    for (const auto &r : static_information.get_relations()) {
        is_predicate_static.push_back(!r.tuples.empty());
    }
}

std::vector<LiftedOperatorId> SuccessorGenerator::get_applicable_actions(const std::vector<
    ActionSchema> &actions, const DBState &state) {
        vector<LiftedOperatorId> applicable_operators;
        // Duplicate code from generic join implementation
        for (const ActionSchema &action : actions) {
            get_applicable_actions(action, state, applicable_operators);
        }
        return applicable_operators;
}


