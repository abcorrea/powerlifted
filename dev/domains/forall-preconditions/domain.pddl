;; Quantified action preconditions without any derived predicate in the
;; PDDL: the universal precondition of stop normalizes to a negated
;; auxiliary axiom, and the existential precondition of grab becomes an
;; extra (non-external) action parameter that must not show up in plans.

(define (domain forall-preconditions)
  (:requirements :strips :typing :universal-preconditions :existential-preconditions)
  (:types item - object)
  (:predicates
    (marked ?x - item)
    (done)
    (grabbed))

  (:action mark
    :parameters (?x - item)
    :precondition (and)
    :effect (marked ?x))

  (:action stop
    :parameters ()
    :precondition (forall (?x - item) (marked ?x))
    :effect (done))

  (:action grab
    :parameters ()
    :precondition (exists (?x - item) (marked ?x))
    :effect (grabbed))
)
