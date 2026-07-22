;; Invalid: an action precondition contains a negated derived predicate.
;; The translator must reject this domain.

(define (domain axioms-invalid-negated-derived-precond)
  (:requirements :strips :typing :derived-predicates)
  (:types item - object)
  (:predicates
    (marked ?x - item)
    (blessed ?x - item)
    (done))

  (:derived (blessed ?x - item)
    (marked ?x))

  (:action finish
    :parameters (?x - item)
    :precondition (not (blessed ?x))
    :effect (done))
)
