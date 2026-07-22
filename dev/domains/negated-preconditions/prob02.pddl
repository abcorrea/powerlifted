;; Unsolvable: only c1 is special but c1 is already marked and can never be
;; unmarked, so (done) is unreachable -- unless negated preconditions are
;; ignored. The reachable states are marking c2 and the three links other
;; than the forbidden (link c1 c2): 2 * 2^3 = 16 states.

(define (problem negated-preconditions-02)
  (:domain negated-preconditions)
  (:objects c1 c2)
  (:init
    (marked c1)
    (special c1)
    (adj c1 c2))
  (:goal (done))
)
