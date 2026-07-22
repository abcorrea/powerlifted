;; Invalid: a universally quantified axiom body normalizes to a negated
;; auxiliary derived predicate, which is outside the supported fragment
;; (this mirrors the 'blocked' axiom of the IPC philosophers domain).
;; The translator must reject this domain.

(define (domain axioms-invalid-forall-body)
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
