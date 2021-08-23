#ifndef SEARCH_DATALOG_DATALOG_H_
#define SEARCH_DATALOG_DATALOG_H_

#include "../task.h"

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
     Atom head;
     vector<Atom> body;
     int action_schema_id;


 */



namespace datalog {

class Datalog {

    // Is this what we want?
    const Task &task;

    void print_parameters(std::vector<Argument> v);

public:
    Datalog(const Task &task);

    void dump_rules();
};

}

#endif //SEARCH_DATALOG_DATALOG_H_
