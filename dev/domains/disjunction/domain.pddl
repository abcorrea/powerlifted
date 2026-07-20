;; Regression domain for issue #64: the translator splits the disjunctive
;; precondition into three action schemas that all keep the name "doit".
;; The Datalog-based heuristics must keep the split schemas apart when
;; building their internal rules.
(define (domain disjunction)
  (:requirements :strips :disjunctive-preconditions)
  (:predicates
    (p ?x)
    (q ?x)
    (r ?x)
    (g ?x))
  (:action doit
    :parameters (?x)
    :precondition (or (p ?x) (q ?x) (r ?x))
    :effect (g ?x))
)
