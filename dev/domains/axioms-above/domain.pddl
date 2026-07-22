;; Blocksworld with a recursive derived predicate (above ?x ?y), used both
;; in the goal (prob01) and in an action precondition (prob02).

(define (domain axioms-above)
  (:requirements :strips :typing :derived-predicates)
  (:types block - object)
  (:predicates
    (on ?x ?y - block)
    (ontable ?x - block)
    (clear ?x - block)
    (handempty)
    (holding ?x - block)
    (above ?x ?y - block)
    (admired))

  (:derived (above ?x ?y - block)
    (on ?x ?y))

  (:derived (above ?x ?y - block)
    (exists (?z - block) (and (on ?x ?z) (above ?z ?y))))

  (:action pick-up
    :parameters (?x - block)
    :precondition (and (clear ?x) (ontable ?x) (handempty))
    :effect (and (not (ontable ?x)) (not (clear ?x)) (not (handempty))
                 (holding ?x)))

  (:action put-down
    :parameters (?x - block)
    :precondition (holding ?x)
    :effect (and (not (holding ?x)) (clear ?x) (handempty) (ontable ?x)))

  (:action stack
    :parameters (?x ?y - block)
    :precondition (and (holding ?x) (clear ?y))
    :effect (and (not (holding ?x)) (not (clear ?y)) (clear ?x) (handempty)
                 (on ?x ?y)))

  (:action unstack
    :parameters (?x ?y - block)
    :precondition (and (on ?x ?y) (clear ?x) (handempty))
    :effect (and (holding ?x) (clear ?y) (not (clear ?x)) (not (handempty))
                 (not (on ?x ?y))))

  ;; Uses the derived predicate in a precondition.
  (:action admire-tower
    :parameters (?x ?y - block)
    :precondition (and (above ?x ?y) (ontable ?y))
    :effect (admired))
)
