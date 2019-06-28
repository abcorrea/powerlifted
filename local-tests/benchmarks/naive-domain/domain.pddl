(define (domain naive)
   (:requirements :strips :typing)
   (:predicates (P ?x)
                (Q ?y)
   )

   (:action set-q
       :parameters  (?x ?y)
       :precondition (and  (P ?x))
       :effect (and  (Q ?y)))

   (:action set-p
       :parameters  (?x ?y)
       :precondition (and  (Q ?x))
       :effect (and  (P ?y)))
)
