;; (p) only becomes derivable once (base) is made true, via the chain
;; base -> q -> p. Optimal plan: make-base, finish (cost 2).

(define (problem axioms-nullary-rec-01)
  (:domain axioms-nullary-rec)
  (:objects)
  (:init)
  (:goal (done))
)
