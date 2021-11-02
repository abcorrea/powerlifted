#ifndef SEARCH_DATALOG_DATALOG_H_
#define SEARCH_DATALOG_DATALOG_H_

#include "rules/rule_base.h"

#include "../atom.h"
#include "../task.h"

#include <map>
#include <memory>
#include <vector>

/*
 * h-add:
R(1, 2)
{V(R(1, 2)) = 0}

Q(X, Y) :- R(X, Y), Aux2()
{Head.V = 1 + Body[0].V + Body[1].V}

Aux2() :- Aux3(X, Y)
{Head.V = 0 + Body[0].V}

@goal-reachable :- ...
{Head.V = 1 + Body[0].V + ... + Body[n].V; Add Head.V to heuristic} # h-add


* rule-based FF
R(1, 2)
{}

Q(X, Y) :- R(X, Y), Aux2()
{Add 1 to heuristic}

Aux2() :- Aux3(X, Y)
{}

@goal-reachable :- ...
{}


* FF
 R(1, 2)
{}

Q(X, Y) :- R(X, Y), Aux2()
{C, D, E = Body[1].Label; add action1(X,Y,C,D,E) to plan}

Aux2() :- Aux3(X, Y)
{E = Body[0].Label; Head.Label = X, Y, E}

@goal-reachable :- ...
{Add cost of plan to heuristic}
 */

/*
{V(R(1, 2)) = 0}
{Head.V = 1 + Body[0].V + Body[1].V}
{Head.V = 1 + Body[0].V + ... + Body[n].V; Add Head.V to heuristic}
{Add 1 to heuristic}
{C, D, E = Body[1].Label; add action1(X,Y,C,D,E) to plan}
{E = Body[0].Label; Head.Label = X, Y, E}
{Add cost of plan to heuristic}
{Add v to heuristic} # h-add if computing V-value during exploration
{Add Head to set of useful atoms}


R(A, B) :- P(A, C), R(B, D), S(B, A, E).
{action(<atom, 0, 0>, <atom, 1, 0>, <atom, 0, 1>, <atom, 1, 1>, <atom, 2, 2>);}

R(1,2).
{return [1, 2]}

R(A, B) :-  R(B, D), S(B, A, E), Aux1(A).
{params = [<3, 0>, <0, 0>, <3, 1>, <0, 1>, <1, 2>];
 emit action(params);
 return params;}
Aux1(A) :- P(A, C)
{params = [<atom, 0, 0>, <atom, 0, 1>];
 emit nothing;
 return params;}


class Annotation;

using Annotation = void(head, rule);

 class FFHeuristic {
vector<int> piff;

 void add_action(head, rule) {...}

 void createDatalog() {
    Annotation f = [](head, rule) {
       piff.push_back(get_action(head, rule));
    }
    Datalog p = create_datalog(add_action);
 }
 }


class ConcatenationAnnotation extends Annotation;
class FFAnnotation extends Annotation;
class RuleBasedFFAnnotation extends Annotation;

class Annotation {

};

// Execution for Rule-based FF annotation
int execute_program_of_node(const GroundAtom &a, Plan &piff) {

}

 // Execution for FF annotation
vector<int = objectID> execute_program_of_node(const GroundAtom &a, Plan &piff) {
  if (is_fact(a)) {
    return a.get_parameters();
  }
  pair<vector<GroundAtom>, Rule> achievers, achieving_rule = atom.get_achiever();
  vector<vector<int>> objects(achievers.size());
  for (const GroundAtom &achiever : achievers) {
     objects[i] = execute_program_of_node(achiever, piff)
  }
  vector<pair<int, int>> parameter_indices = achieving_rule.param_indices;
  vector<int = objectID> parameters(parameter_indices.size());
  for (pair<int, int> idx_pair : parameter_indices) {
     int body_atom_idx = idx_pair.first;
     int body_parameter_position = idx_pair.second
     parameters[i] = objects[body_atom_idx][body_parameter_position];
  }
  if (achieving_rule.action_to_emit != -1) {
      GroundAction ground_action(achieving_rule.action_to_emit, parameters);
      piff.insert(ground_action);
  }
  return parameters;
}




 R(A, B) :- P(A, C), R(B, D), S(B, A, E).
 {add action(A, B, C, D, E)}

 R(A, B) :- Aux1(A), R(B, D), S(B, A, E).
 {C = Body[0].Label[1]; add action(A, B, C, D, E)}
 Aux1(A) :- P(A, C)
 {Head.Label = A, C}
 P(A, C) :- Aux7(C), M(A, C).



 R(X, Y) :- Aux1(X), Aux2(Y, X)
 Aux1(X) :- P(X, Y)
 Aux2(X, Y) :- R(X, Z), Aux3(Y, W)
 Aux3(X, Y) :- S(X, Y, Z)


class GroundAtom:
     int v;



class Rule:
     DatalogAtom head;
     vector<DatalogAtom> body;
     int action_schema_id;


 */



namespace datalog {

class Datalog {

    std::vector<Fact> permanent_edb;
    std::vector<std::unique_ptr<RuleBase>> rules;

    // Is this what we want?
    const Task &task;

    int goal_atom_idx;

    std::vector<std::string> predicate_names;
    std::unordered_map<std::string, int> map_new_predicates_to_idx;

    void create_rules(AnnotationGenerator ann);
    void generate_action_rule(const ActionSchema &schema, std::vector<size_t> nullary_preconds, AnnotationGenerator &annotation_generator);

    void generate_action_effect_rules(const ActionSchema &schema, AnnotationGenerator &annotation_generator);

    std::vector<DatalogAtom> get_action_effect_rule_body(const ActionSchema &schema);
    void get_nullary_atoms_from_vector(const std::vector<bool> &nullary_predicates_in_precond,
                                       std::vector<size_t> &nullary_preconds) const;
    std::vector<DatalogAtom> get_atoms_in_rule_body(const ActionSchema &schema,
                                                    const std::vector<size_t> &nullary_preconds) const;


    int get_next_auxiliary_predicate_idx() {
        return predicate_names.size();
    }

    void output_rule(const std::unique_ptr<RuleBase> &rule);
    void output_atom(const DatalogAtom &atom);
    void output_parameters(const Arguments& v);

    void set_permanent_edb(StaticInformation static_information);
    void get_always_reachable_rule_heads();
    void output_permanent_edb();

    void add_goal_rule(const Task &task, AnnotationGenerator &annotation_generator);

public:
    Datalog(const Task &task, AnnotationGenerator annotation_generator);

    std::vector<std::unique_ptr<RuleBase>> &get_rules() {
        return rules;
    }
};

}

#endif //SEARCH_DATALOG_DATALOG_H_
