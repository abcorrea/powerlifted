#ifndef SEARCH_TASK_H
#define SEARCH_TASK_H

#include <string>
#include <vector>
#include <unordered_map>
#include <ostream>

#include "predicate.h"
#include "object.h"
#include "state.h"
#include "goal_condition.h"
#include "action_schema.h"

class Task {

public:

    std::vector<Predicate> predicates;
    std::vector<Object> objects;
    State initial_state; // TODO setter
    GoalCondition goal;
    std::vector<ActionSchema> actions;

    Task(const std::string& domain_name, const std::string& task_name) :
            domain_name(domain_name), task_name(task_name) {
        // Create class only with task and domain names
    }

    const std::string &getDomainName() const {
        return domain_name;
    }

    const std::string &getTaskName() const {
        return task_name;
    }

    const void addType(const std::string& type_name);

    const void
    addPredicate(const std::string &name, int index, int arity, bool static_predicate, const std::vector<int> &types);

    const void addObject(const std::string& name, int index, const std::vector<int>& types);

    void initializeEmptyInitialState();

    void initializeGoal(std::vector<AtomicGoal> goals);

    void initializeActionSchemas(const std::vector<ActionSchema> &action_list);

    void dumpState(State s) const;

    void dumpGoal();

private:
    const std::string &domain_name;
    const std::string &task_name;

    std::vector<std::string> type_names;
};


#endif //SEARCH_TASK_H
