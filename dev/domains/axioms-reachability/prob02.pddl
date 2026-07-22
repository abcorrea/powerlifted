;; Disjunctive goal over derived atoms: exercises the auxiliary goal axiom
;; created by the translator (a nullary derived predicate defined by two
;; axioms whose bodies contain derived atoms and constants). The optimal
;; plan builds the single road n1 -> n3 (cost 1).

(define (problem axioms-reachability-02)
  (:domain axioms-reachability)
  (:objects n1 n2 n3 n4 - node)
  (:init
    (edge n1 n2)
    (edge n2 n3)
    (edge n1 n3)
    (edge n3 n4))
  (:goal (or (reachable n1 n3) (reachable n1 n4)))
)
