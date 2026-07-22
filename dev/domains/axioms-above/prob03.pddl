;; Unsolvable instance used to test that derived atoms disappear when their
;; supporting base facts are deleted (unstack removes (on ?x ?y)). The goal
;; (above a a) can never hold. With two blocks there are exactly 5 reachable
;; base configurations {both on table, a on b, b on a, holding a, holding b},
;; each with and without the (admired) flag reachable via admire-tower, i.e.
;; 10 states in total. Stale derived atoms surviving an unstack would make
;; equal base states look different and raise the registered-state count.

(define (problem axioms-above-03)
  (:domain axioms-above)
  (:objects a b - block)
  (:init
    (ontable a) (ontable b)
    (clear a) (clear b)
    (handempty))
  (:goal (above a a))
)
