;; Solvable: c2 is special and unmarked, so finish c2 works; (linked c2 c1)
;; is allowed since only (adj c1 c2) holds. Optimal plan: finish c2,
;; link c2 c1 (cost 2).

(define (problem negated-preconditions-01)
  (:domain negated-preconditions)
  (:objects c1 c2)
  (:init
    (marked c1)
    (special c2)
    (adj c1 c2))
  (:goal (and (done) (linked c2 c1)))
)
