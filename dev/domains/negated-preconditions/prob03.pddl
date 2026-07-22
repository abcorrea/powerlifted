;; Negated fluent atom in the goal whose positive counterpart is not even
;; relaxed-reachable: (cursed ?x) is only added by curse, which needs a
;; self-loop (adj ?x ?x) that no instance provides. The Datalog heuristics
;; must relax the negated goal atom away instead of requiring cursed(c2) to
;; be reachable. Optimal plan: finish c2 (cost 1).

(define (problem negated-preconditions-03)
  (:domain negated-preconditions)
  (:objects c1 c2)
  (:init
    (marked c1)
    (special c2)
    (adj c1 c2))
  (:goal (and (done) (not (cursed c2))))
)
