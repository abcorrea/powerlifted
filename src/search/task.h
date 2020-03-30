#ifndef SEARCH_TASK_H
#define SEARCH_TASK_H

#include "action_schema.h"
#include "goal_condition.h"
#include "object.h"
#include "predicate.h"
#include "state.h"

#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>


/**
 * @brief Represents the task, with all its objects, goals, action schemas, etc.
 * Variable and function names should be self-explanatory.
 *
 * @attention The initial state and the static info have info for all predicates.
 * However, the static predicates will always be empty in any state and the fluents are
 * always empty in the static_info variable.
 *
 */

class Task {

public:
  std::vector<Predicate> predicates;
  std::vector<Object> objects;
  State initial_state;
  StaticInformation static_info;
  GoalCondition goal;
  std::vector<ActionSchema> actions;
  std::vector<std::string> type_names;
  std::unordered_set<int> nullary_predicates;

  Task(const std::string &domain_name, const std::string &task_name)
      : domain_name(domain_name), task_name(task_name) {
    // Create class only with task and domain names
  }

  const std::string &get_domain_name() const { return domain_name; }

  const std::string &get_task_name() const { return task_name; }

  const void add_type(const std::string &type_name);

  const void add_predicate(std::string &name, int index, int arity,
                           bool static_predicate, std::vector<int> &types);

  const void add_object(const std::string &name, int index,
                        const std::vector<int> &types);

  void create_empty_initial_state();

  void create_goal_condition(std::vector<AtomicGoal> goals,
                             std::unordered_set<int> nullary_goals,
                             std::unordered_set<int> negative_nullary_goals);

  void initialize_action_schemas(const std::vector<ActionSchema> &action_list);

  void dump_state(State s) const;

  void dump_goal();

  bool is_goal(const State &state, const GoalCondition &goal_condition) const;

  bool is_trivially_unsolvable() const;

private:
  const std::string &domain_name;
  const std::string &task_name;
};

#endif // SEARCH_TASK_H
