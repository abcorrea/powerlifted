;; Invalid: axiom body contains a negated (non-equality) atom.
;; The translator must reject this domain.

(define (domain axioms-invalid-negated-body)
  (:requirements :strips :typing :derived-predicates)
  (:types item - object)
  (:predicates
    (marked ?x - item)
    (unmarked ?x - item)
    (done))

  (:derived (unmarked ?x - item)
    (not (marked ?x)))

  (:action mark
    :parameters (?x - item)
    :precondition (and)
    :effect (marked ?x))

  (:action finish
    :parameters (?x - item)
    :precondition (unmarked ?x)
    :effect (done))
)
