#include "task.h"

const void
Task::addPredicate(const std::string &name, int index, int arity, bool static_predicate,
                   const std::vector<int> &types) {
    Task::predicates.emplace_back(name, index, arity, static_predicate, types);
}

const void Task::addObject(const std::string &name, int index, const std::vector<int> &types) {
    Task::objects.emplace_back(name, index, types);
}

const void Task::addType(const std::string &type_name) {
    type_names.push_back(type_name);
}
