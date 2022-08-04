#include "task.h"

#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

using namespace std;

void Task::add_predicate(
    string &name, int index, int arity, bool static_predicate, vector<int> &types)
{
    predicates.emplace_back(std::move(name), index, arity, static_predicate, std::move(types));
}

void Task::add_object(const string &name, int index, const vector<int> &types)
{
    objects.emplace_back(name, index, types);
}

void Task::add_type(const string &type_name) { type_names.push_back(type_name); }

void Task::create_empty_initial_state(size_t number_predicates)
{
    /*
     * Creates one empty relation for every predicate of the task in the initial
     * state.
     */
    vector<Relation> fluents, static_preds;
    for (size_t i = 0; i < predicates.size(); ++i) {
        Relation r;
        r.predicate_symbol = i;
        assert(r.tuples.empty());
        static_preds.push_back(r);
        fluents.push_back(r);
    }
    initial_state = DBState(std::move(fluents), vector<bool>(predicates.size(), false));
    static_info = StaticInformation(std::move(static_preds), vector<bool>(predicates.size(), false));
}

void Task::dump_state(DBState s) const
{
    /*
     * Output initial state in a human readable way.
     */
    const auto& nullary_atoms = s.get_nullary_atoms();
    for (size_t j = 0; j < nullary_atoms.size(); ++j) {
        if (nullary_atoms[j])
            cout << predicates[j].get_name() << ", ";
    }
    const auto& relations = s.get_relations();
    for (size_t i = 0; i < relations.size(); ++i) {
        string relation_name = predicates[i].get_name();
        unordered_set<GroundAtom, TupleHash> tuples = relations[i].tuples;
        for (auto &tuple : tuples) {
            cout << relation_name << "(";
            for (auto obj : tuple) {
                cout << objects[obj].get_name() << ",";
            }
            cout << "), ";
        }
    }
    cout << endl;
}

void Task::dump_goal()
{
    /*
     * Output goal condition in a readable format.
     */
    for (int g : goal.positive_nullary_goals) {
        cout << predicates[g].get_name() << endl;
    }
    for (int g : goal.negative_nullary_goals) {
        cout << "Not " << predicates[g].get_name() << endl;
    }
    for (const auto &g : goal.goal) {
        if (g.is_negated()) {
            cout << "Not ";
        }
        cout << predicates[g.get_predicate_index()].get_name() << " ";
        for (int arg : g.get_arguments()) {
            cout << objects[arg].get_name() << " ";
        }
        cout << endl;
    }
}


void Task::create_goal_condition(std::vector<AtomicGoal> goals,
                                 std::unordered_set<int> nullary_goals,
                                 std::unordered_set<int> negative_nullary_goals)
{
    goal = GoalCondition(
        std::move(goals), std::move(nullary_goals), std::move(negative_nullary_goals));
}

void Task::initialize_action_schemas(const std::vector<ActionSchema> &action_list)
{
    action_schemas = action_list;
}

bool Task::is_goal(const DBState &state) const
{
    for (int pred : goal.positive_nullary_goals) {
        if (!state.get_nullary_atoms()[pred])
            return false;
    }
    for (int pred : goal.negative_nullary_goals) {
        if (state.get_nullary_atoms()[pred])
            return false;
    }
    for (const AtomicGoal &atomicGoal : goal.goal) {
        int goal_predicate = atomicGoal.get_predicate_index();
        const Relation &relation_at_goal_predicate = state.get_relations()[goal_predicate];

        assert(!predicates[relation_at_goal_predicate.predicate_symbol].isStaticPredicate());
        assert(goal_predicate == relation_at_goal_predicate.predicate_symbol);

        const auto it = relation_at_goal_predicate.tuples.find(atomicGoal.get_arguments());
        const auto end = relation_at_goal_predicate.tuples.end();
        if ((!atomicGoal.is_negated() && it == end) || (atomicGoal.is_negated() && it != end)) {
            return false;
        }
    }
    return true;
}

bool Task::is_trivially_unsolvable() const
{
    /*
     * Checks whether the static conditions in the goal condition are not
     * satisfied.
     *
     * This should be guaranteed by the translator. Just a safety check.
     */
    for (const AtomicGoal &atomicGoal : goal.goal) {
        int goal_predicate = atomicGoal.get_predicate_index();
        Relation relation_at_goal_predicate = static_info.get_relations()[goal_predicate];
        if (!predicates[relation_at_goal_predicate.predicate_symbol].isStaticPredicate())
            continue;
        assert(goal_predicate == relation_at_goal_predicate.predicate_symbol);

        const auto it = relation_at_goal_predicate.tuples.find(atomicGoal.get_arguments());
        const auto end = relation_at_goal_predicate.tuples.end();
        if ((!atomicGoal.is_negated() && it == end) || (atomicGoal.is_negated() && it != end)) {
            return true;
        }
    }
    return false;
}


std::vector<std::vector<int>> Task::compute_object_index() const {
    std::vector<std::vector<int>> objects_per_type(type_names.size());

    for (const Object &o:objects) {
        for (int t : o.get_types()) {
            objects_per_type[t].push_back(o.get_index());
        }
    }

    return objects_per_type;
}
