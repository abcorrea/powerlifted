;; The negated derived precondition of rescue holds initially (c1 is not
;; safe); optimal plan: rescue c1 (cost 1).

(define (problem axioms-negation-02)
  (:domain axioms-negation)
  (:objects c1 - item)
  (:init)
  (:goal (rescued c1))
)
