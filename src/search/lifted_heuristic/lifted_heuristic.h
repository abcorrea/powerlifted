#ifndef SEARCH_HEURISTICS_LIFTED_HEURISTIC_H_
#define SEARCH_HEURISTICS_LIFTED_HEURISTIC_H_

#include "fact.h"
#include "logic_program.h"

#include "grounders/weighted_grounder.h"

#include "../task.h"

#include "../heuristics/heuristic.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

class MapPlanningTaskToLP {
    /*
     Maps elements of the planning task to the elements of the LP (objects, predicate symbols, etc.)

     The 'inverse' structures do the inverse mapping: from the LP elements to the planning task
     elements.
     */
    std::unordered_map<int, int> object_map;
    std::unordered_map<int, int> inverse_object_map;
    std::unordered_map<int, int> predicate_symbol_map;
    std::unordered_map<int, int> inverse_predicate_symbol_map;

public:
    MapPlanningTaskToLP() = default;

    void add_object_mapping(int i, int j) {
        assert (object_map.find(i) == object_map.end());
        object_map[i] = j;
        inverse_object_map[j] = i;
    }

    int get_object(int i) {
        assert (object_map.find(i) != object_map.end());
        return object_map.at(i);
    }

    int get_inverse_object(int i) {
        assert (inverse_object_map.find(i) != inverse_object_map.end());
        return inverse_object_map.at(i);
    }

    void add_predicate_mapping(int i, int j) {
        assert(predicate_symbol_map.find(i) == predicate_symbol_map.end());
        predicate_symbol_map[i] = j;
        inverse_predicate_symbol_map[j] = i;
    }

    int get_predicate(int i) {
        assert (predicate_symbol_map.find(i) != predicate_symbol_map.end());
        return predicate_symbol_map.at(i);
    }

    int get_inverse_predicate(int i) {
        assert (inverse_predicate_symbol_map.find(i) != inverse_predicate_symbol_map.end());
        return inverse_predicate_symbol_map.at(i);
    }

    bool is_auxiliary_predicate(int i) const {
        return (inverse_predicate_symbol_map.find(i) == inverse_predicate_symbol_map.end());
    }
};


class LiftedHeuristic : public Heuristic {
    lifted_heuristic::LogicProgram logic_program;
    lifted_heuristic::WeightedGrounder grounder;

    MapPlanningTaskToLP indices_map;

    int base_fact_index;
    int target_predicate;

    void transform_state_into_edb(
        const DBState &s,
        const std::unordered_set<int> &nullaries);

public:
    LiftedHeuristic(const Task &task, const std::string &datalog_file, int heuristic_type);

    int compute_heuristic(const DBState &s, const Task &task) final;
    void get_useful_facts(const Task &task, const lifted_heuristic::LogicProgram &lp);
};

#endif //SEARCH_HEURISTICS_LIFTED_HEURISTIC_H_
