;; Multi-stratum axioms: (doubly-marked ?x) depends (non-recursively) on the
;; derived predicate (marked-pair ?x ?y), which itself has an inequality
;; literal in its body; (linked ?x c3) has a constant in the body once the
;; instances name the object c3.

(define (domain axioms-strata)
  (:requirements :strips :typing :equality :derived-predicates)
  (:types item - object)
  (:constants c3 - item)
  (:predicates
    (marked ?x - item)
    (linked ?x ?y - item)
    (marked-pair ?x ?y - item)
    (special ?x - item)
    (done))

  ;; Stratum 0: two distinct marked items.
  (:derived (marked-pair ?x ?y - item)
    (and (marked ?x) (marked ?y) (not (= ?x ?y))))

  ;; Stratum 1: builds on marked-pair; the body also has a constant (c3)
  ;; in the instances via the static predicate linked.
  (:derived (special ?x - item)
    (exists (?y - item) (and (marked-pair ?x ?y) (linked ?x c3))))

  (:action mark
    :parameters (?x - item)
    :precondition (and)
    :effect (marked ?x))

  (:action finish
    :parameters (?x - item)
    :precondition (special ?x)
    :effect (done))
)
