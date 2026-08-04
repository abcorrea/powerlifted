;; Unsolvable instance used to test duplicate detection under axioms: roads
;; can only be built along the four directed edges, so (reachable n4 n1) can
;; never hold, and the reachable state space is exactly the 2^4 = 16 subsets
;; of buildable roads. Since the derived (reachable ...) atoms are a function
;; of the built roads, an exhaustive blind search must register exactly 16
;; states; a higher count would mean states differing only in derived atoms
;; were treated as distinct.

(define (problem axioms-reachability-03)
  (:domain axioms-reachability)
  (:objects n1 n2 n3 n4 - node)
  (:init
    (edge n1 n2)
    (edge n2 n3)
    (edge n1 n3)
    (edge n3 n4))
  (:goal (reachable n4 n1))
)
