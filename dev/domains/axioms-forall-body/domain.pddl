;; A universally quantified axiom body (this mirrors the 'blocked' axiom of
;; the IPC philosophers domain): (all-marked) normalizes to the negation of
;; an auxiliary derived predicate whose body has a negated fluent.

(define (domain axioms-forall-body)
  (:requirements :strips :typing :derived-predicates)
  (:types item - object)
  (:predicates
    (marked ?x - item)
    (all-marked)
    (done))

  (:derived (all-marked)
    (forall (?x - item) (marked ?x)))

  (:action mark
    :parameters (?x - item)
    :precondition (and)
    :effect (marked ?x))

  (:action finish
    :parameters ()
    :precondition (all-marked)
    :effect (done))
)
