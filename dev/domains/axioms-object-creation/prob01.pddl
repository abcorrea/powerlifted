;; Optimal plan: spawn a, spawn the created object, then finish with the
;; two-link chain (cost 3). reach(a, second-created) only holds through
;; the recursive rule, and (not (linked ...)) rules the one-link pairs out.

(define (problem oc-axioms-01)
  (:domain oc-axioms)
  (:objects a - item)
  (:init (have a))
  (:goal (done))
)
