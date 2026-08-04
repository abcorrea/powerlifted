;; Stratified negation: unmarked (stratum 0) negates a fluent, exposed
;; (stratum 1) combines a derived atom with a negated fluent, and safe
;; (stratum 2) negates a derived predicate. rescue uses a negated derived
;; predicate in a precondition.

(define (domain axioms-negation)
  (:requirements :strips :typing :negative-preconditions :derived-predicates)
  (:types item - object)
  (:predicates
    (marked ?x - item)
    (protected ?x - item)
    (unmarked ?x - item)
    (exposed ?x - item)
    (safe ?x - item)
    (rescued ?x - item)
    (done))

  (:derived (unmarked ?x - item)
    (not (marked ?x)))

  (:derived (exposed ?x - item)
    (and (unmarked ?x) (not (protected ?x))))

  (:derived (safe ?x - item)
    (not (exposed ?x)))

  (:action mark
    :parameters (?x - item)
    :precondition (and)
    :effect (marked ?x))

  (:action shield
    :parameters (?x - item)
    :precondition (and)
    :effect (protected ?x))

  (:action rescue
    :parameters (?x - item)
    :precondition (not (safe ?x))
    :effect (rescued ?x))

  (:action finish
    :parameters (?x - item)
    :precondition (safe ?x)
    :effect (done))
)
