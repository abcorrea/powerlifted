;; Regression domain for (in)equality filters on join programs with a
;; single table: pick's join program consists only of the type atom of ?x,
;; so the inequality must be enforced outside the join loop.

(define (domain single-table-inequality)
  (:requirements :strips :equality)
  (:predicates (chosen ?x))
  (:action pick
    :parameters (?x)
    :precondition (not (= ?x c1))
    :effect (chosen ?x))
)
