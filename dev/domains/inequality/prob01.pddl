;; (chosen c1) can never hold since pick requires ?x != c1; the reachable
;; states are only {} and {(chosen c2)}. An invalid plan picking c1 means
;; the inequality filter was skipped.

(define (problem single-table-inequality-01)
  (:domain single-table-inequality)
  (:objects c1 c2)
  (:init)
  (:goal (chosen c1))
)
