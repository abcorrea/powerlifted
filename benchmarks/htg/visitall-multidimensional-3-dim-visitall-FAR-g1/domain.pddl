
(define (domain visitall-3-dim)
(:requirements :strips :equality :typing)
(:types        pos - object)
(:predicates 
	     (at-robot ?x0 ?x1 ?x2 - pos)
	     (visited ?x0 ?x1 ?x2 - pos)
         (neighbor ?p1 ?p2 - pos)
)


(:action move-0
:parameters (?x0_from ?x1_from ?x2_from ?x0_to - pos)
:precondition (and (at-robot ?x0_from ?x1_from ?x2_from)  (neighbor ?x0_from ?x0_to))
:effect (and (at-robot ?x0_to ?x1_from ?x2_from) (visited ?x0_to ?x1_from ?x2_from) (not (at-robot ?x0_from ?x1_from ?x2_from)))
)

(:action move-1
:parameters (?x0_from ?x1_from ?x2_from ?x1_to - pos)
:precondition (and (at-robot ?x0_from ?x1_from ?x2_from)  (neighbor ?x1_from ?x1_to))
:effect (and (at-robot ?x0_from ?x1_to ?x2_from) (visited ?x0_from ?x1_to ?x2_from) (not (at-robot ?x0_from ?x1_from ?x2_from)))
)

(:action move-2
:parameters (?x0_from ?x1_from ?x2_from ?x2_to - pos)
:precondition (and (at-robot ?x0_from ?x1_from ?x2_from)  (neighbor ?x2_from ?x2_to))
:effect (and (at-robot ?x0_from ?x1_from ?x2_to) (visited ?x0_from ?x1_from ?x2_to) (not (at-robot ?x0_from ?x1_from ?x2_from)))
)


)

