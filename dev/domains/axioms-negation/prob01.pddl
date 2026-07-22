;; Initially c1 is unmarked and unprotected, hence exposed and not safe.
;; Marking or shielding it makes it safe; optimal plan: mark c1, finish c1
;; (cost 2).

(define (problem axioms-negation-01)
  (:domain axioms-negation)
  (:objects c1 - item)
  (:init)
  (:goal (done))
)
