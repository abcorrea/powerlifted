;; Ordering matters: rescue only works while c1 is not safe, finish only
;; once it is. Optimal plan: rescue c1, mark c1 (or shield c1), finish c1
;; (cost 3).

(define (problem axioms-negation-03)
  (:domain axioms-negation)
  (:objects c1 - item)
  (:init)
  (:goal (and (rescued c1) (done)))
)
