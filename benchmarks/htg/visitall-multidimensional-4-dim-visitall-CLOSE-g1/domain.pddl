
(define (domain visitall-4-dim)
(:requirements :strips :equality :typing)
(:types        pos - object)
(:predicates 
	     (at-robot ?x0 ?x1 ?x2 ?x3 - pos)
	     (visited ?x0 ?x1 ?x2 ?x3 - pos)
         (neighbor ?p1 ?p2 - pos)
)


(:action move-0
:parameters (?x0_from ?x1_from ?x2_from ?x3_from ?x0_to - pos)
:precondition (and (at-robot ?x0_from ?x1_from ?x2_from ?x3_from)  (neighbor ?x0_from ?x0_to))
:effect (and (at-robot ?x0_to ?x1_from ?x2_from ?x3_from) (visited ?x0_to ?x1_from ?x2_from ?x3_from) (not (at-robot ?x0_from ?x1_from ?x2_from ?x3_from)))
)

(:action move-1
:parameters (?x0_from ?x1_from ?x2_from ?x3_from ?x1_to - pos)
:precondition (and (at-robot ?x0_from ?x1_from ?x2_from ?x3_from)  (neighbor ?x1_from ?x1_to))
:effect (and (at-robot ?x0_from ?x1_to ?x2_from ?x3_from) (visited ?x0_from ?x1_to ?x2_from ?x3_from) (not (at-robot ?x0_from ?x1_from ?x2_from ?x3_from)))
)

(:action move-2
:parameters (?x0_from ?x1_from ?x2_from ?x3_from ?x2_to - pos)
:precondition (and (at-robot ?x0_from ?x1_from ?x2_from ?x3_from)  (neighbor ?x2_from ?x2_to))
:effect (and (at-robot ?x0_from ?x1_from ?x2_to ?x3_from) (visited ?x0_from ?x1_from ?x2_to ?x3_from) (not (at-robot ?x0_from ?x1_from ?x2_from ?x3_from)))
)

(:action move-3
:parameters (?x0_from ?x1_from ?x2_from ?x3_from ?x3_to - pos)
:precondition (and (at-robot ?x0_from ?x1_from ?x2_from ?x3_from)  (neighbor ?x3_from ?x3_to))
:effect (and (at-robot ?x0_from ?x1_from ?x2_from ?x3_to) (visited ?x0_from ?x1_from ?x2_from ?x3_to) (not (at-robot ?x0_from ?x1_from ?x2_from ?x3_from)))
)


)

