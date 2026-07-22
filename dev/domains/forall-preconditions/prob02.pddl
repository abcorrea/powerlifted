;; Optimal plan: mark c1 (or c2), grab (cost 2). The existentially
;; quantified variable of grab is not part of the ground action name, so
;; the plan must print (grab) without arguments to validate.

(define (problem forall-preconditions-02)
  (:domain forall-preconditions)
  (:objects c1 c2 - item)
  (:init)
  (:goal (grabbed))
)
