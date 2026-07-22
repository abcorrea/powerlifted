;; Invalid: p and q negate each other, so the strongly connected component
;; {p, q} of the dependency graph contains negative edges. The translator
;; must reject this domain as not stratifiable.

(define (domain axioms-invalid-not-stratifiable)
  (:requirements :strips :typing :derived-predicates)
  (:types item - object)
  (:predicates
    (marked ?x - item)
    (p ?x - item)
    (q ?x - item)
    (done))

  (:derived (p ?x - item)
    (not (q ?x)))

  (:derived (q ?x - item)
    (not (p ?x)))

  (:action finish
    :parameters (?x - item)
    :precondition (p ?x)
    :effect (done))
)
