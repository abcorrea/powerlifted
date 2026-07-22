;; Goal (done) requires (special ?x) for some x, which requires two distinct
;; marked items, one of which is linked to the constant c3. Only c1 is
;; linked to c3, so the optimal plan marks c1 plus one other item and
;; finishes: mark c1, mark c2 (or c3), finish c1 (cost 3).

(define (problem axioms-strata-01)
  (:domain axioms-strata)
  (:objects c1 c2 - item)
  (:init
    (linked c1 c3))
  (:goal (done))
)
