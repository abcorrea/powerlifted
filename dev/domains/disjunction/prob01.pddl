;; The goal needs two different disjuncts of the split schema: (g a) is only
;; reachable through the q-disjunct and (g b) only through the r-disjunct.
;; With the issue #64 name collision, at most one disjunct kept its effects,
;; so the Datalog heuristics reported this solvable task as unsolvable.
(define (problem disjunction-01)
  (:domain disjunction)
  (:objects a b)
  (:init (q a) (r b))
  (:goal (and (g a) (g b)))
)
