;; Derived predicate in an action precondition: (admire-tower ?x ?y)
;; requires (above ?x ?y). Starting from the tower c on b on a (a on the
;; table), admire-tower c a is applicable immediately thanks to the
;; recursive rule; the optimal plan is a single action (cost 1).

(define (problem axioms-above-02)
  (:domain axioms-above)
  (:objects a b c - block)
  (:init
    (ontable a) (on b a) (on c b)
    (clear c)
    (handempty))
  (:goal (admired))
)
