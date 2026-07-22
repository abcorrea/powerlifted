;; Negated precondition atoms over fluent (marked) and static (adj)
;; predicates, enforced as anti-join filters by the join-based successor
;; generators.

(define (domain negated-preconditions)
  (:requirements :strips :negative-preconditions)
  (:predicates
    (marked ?x)
    (special ?x)
    (adj ?x ?y)
    (linked ?x ?y)
    (cursed ?x)
    (done))

  (:action mark
    :parameters (?x)
    :precondition (not (marked ?x))
    :effect (marked ?x))

  (:action finish
    :parameters (?x)
    :precondition (and (special ?x) (not (marked ?x)))
    :effect (done))

  (:action link
    :parameters (?x ?y)
    :precondition (not (adj ?x ?y))
    :effect (linked ?x ?y))

  ;; Only applicable with a self-loop in adj; the instances provide none,
  ;; so (cursed ?x) stays unreachable even in the delete relaxation.
  (:action curse
    :parameters (?x)
    :precondition (adj ?x ?x)
    :effect (cursed ?x))
)
