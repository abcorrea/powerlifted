;; Invalid: a derived predicate appears in an action effect.
;; The translator must reject this domain.

(define (domain axioms-invalid-derived-in-effect)
  (:requirements :strips :typing :derived-predicates)
  (:types item - object)
  (:predicates
    (marked ?x - item)
    (blessed ?x - item)
    (done))

  (:derived (blessed ?x - item)
    (marked ?x))

  (:action bless-directly
    :parameters (?x - item)
    :precondition (and)
    :effect (blessed ?x))

  (:action finish
    :parameters (?x - item)
    :precondition (blessed ?x)
    :effect (done))
)
