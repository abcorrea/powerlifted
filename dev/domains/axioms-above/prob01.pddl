;; Derived predicate in the goal: get block a above block c.
;; Blocks a, b, c start on the table; the optimal plan is
;; pick-up a, stack a c (cost 2).

(define (problem axioms-above-01)
  (:domain axioms-above)
  (:objects a b c - block)
  (:init
    (ontable a) (ontable b) (ontable c)
    (clear a) (clear b) (clear c)
    (handempty))
  (:goal (above a c))
)
