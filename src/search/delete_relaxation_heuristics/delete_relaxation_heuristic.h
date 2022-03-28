#ifndef SEARCH_DELETE_RELAXATION_HEURISTICS_DELETE_RELAXATION_HEURISTIC_H_
#define SEARCH_DELETE_RELAXATION_HEURISTICS_DELETE_RELAXATION_HEURISTIC_H_

class DeleteRelaxationHeuristic;
/*
class DeleteRelaxationHeuristic {
    DatalogProgram datalog;
    void backtrack(const GroundAtom &a);
protected:
    virtual void handle_node(const GroundAtom &a, const Rule &achiever, const vector<int> &achiever_body);
    virtual int get_heuristic_value();
public:
    int compute_heuristic_value() {
        compute_hadd_achievers();
        backtrack(goal);
        get_heuristic_value();
    }
};

class FFHeuristic {
    Plan piFF;
protected:
    void handle_node(const GroundAtom &a, const Rule &achiever, const vector<int> &achiever_body) override;
public:
    int get_heuristic_value() {
        piFF.clear();
        compute_hadd_achievers_and_backtrack(goal);
        return cost(piFF);
        vector<ObjectID> execute_ff_program_of_node(const GroundAtom &a, Plan &piff);
    }
};


class RuleBasedFFHeuristic {

    vector<ObjectID> execute_rule_based_program_of_node(const GroundAtom &a, Plan &piff);

public:
    int get_heuristic_value() {
        int execute_rule_based_program_of_node(const GroundAtom &a, Plan &piff);
    }
};

class LandmarkHeuristic {

    vector<ObjectID> execute_rule_based_program_of_node(const GroundAtom &a, Plan &piff);

public:
    void get_landmarks() {
        vector<Landmarks> execute_rule_based_program_of_node(const GroundAtom &a, Plan &piff);
    }
};
*/


#endif //SEARCH_DELETE_RELAXATION_HEURISTICS_DELETE_RELAXATION_HEURISTIC_H_
