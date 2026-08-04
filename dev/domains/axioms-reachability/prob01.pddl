;; Chain n1 - n2 - n3 - n4 with a shortcut n1 - n3. The goal is a derived
;; atom; the optimal plan builds the two roads n1 -> n3 -> n4 (cost 2).

(define (problem axioms-reachability-01)
  (:domain axioms-reachability)
  (:objects n1 n2 n3 n4 - node)
  (:init
    (edge n1 n2)
    (edge n2 n3)
    (edge n1 n3)
    (edge n3 n4))
  (:goal (reachable n1 n4))
)
