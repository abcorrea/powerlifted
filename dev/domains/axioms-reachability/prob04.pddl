;; A pure chain n1 -> ... -> n8: reaching n8 from n1 requires building all
;; seven roads, and the transitive-closure fixpoint over the final state
;; needs seven rounds -- this exercises the semi-naive delta iteration of
;; the axiom evaluator. Optimal plan cost 7.

(define (problem axioms-reachability-04)
  (:domain axioms-reachability)
  (:objects n1 n2 n3 n4 n5 n6 n7 n8 - node)
  (:init
    (edge n1 n2)
    (edge n2 n3)
    (edge n3 n4)
    (edge n4 n5)
    (edge n5 n6)
    (edge n6 n7)
    (edge n7 n8))
  (:goal (reachable n1 n8))
)
