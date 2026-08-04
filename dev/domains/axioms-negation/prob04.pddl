;; Universally quantified goal: normalizes to a negated auxiliary derived
;; atom in the goal. Optimal plan: mark c1, mark c2 (cost 2).

(define (problem axioms-negation-04)
  (:domain axioms-negation)
  (:objects c1 c2 - item)
  (:init)
  (:goal (forall (?x - item) (marked ?x)))
)
