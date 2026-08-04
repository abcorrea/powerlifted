;; Axioms combined with object creation: spawn creates a fresh object
;; linked to its creator, and the recursive derived predicate reach must
;; be derived through the created objects. finish additionally exercises
;; a negated fluent precondition and an inequality over fresh objects.

(define (domain oc-axioms)
  (:requirements :strips :typing :negative-preconditions :derived-predicates)
  (:types item - object)
  (:predicates
    (have ?x - item)
    (linked ?x - item ?y - item)
    (reach ?x - item ?y - item)
    (done))

  (:derived (reach ?x - item ?y - item)
    (linked ?x ?y))
  (:derived (reach ?x - item ?y - item)
    (exists (?z - item) (and (linked ?x ?z) (reach ?z ?y))))

  (:action spawn
    :parameters (?x - item)
    :precondition (have ?x)
    :effect (:new (?o - item) (and (have ?o) (linked ?x ?o))))

  (:action finish
    :parameters (?x - item ?y - item)
    :precondition (and (reach ?x ?y) (not (linked ?x ?y)) (not (= ?x ?y)))
    :effect (done))
)
