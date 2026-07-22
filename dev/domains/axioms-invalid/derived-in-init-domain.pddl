;; Invalid: a derived predicate appears in the initial state.
;; The translator must reject this task.

(define (domain axioms-invalid-derived-in-init)
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
    :precondition (blessed ?x)
    :effect (done))
)
