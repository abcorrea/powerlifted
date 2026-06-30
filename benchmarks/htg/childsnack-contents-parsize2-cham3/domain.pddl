
(define (domain htg-child-snack)
(:requirements :typing :equality)
(:types child bread-portion content-portion content-description sandwich tray place)
(:constants kitchen - place)

(:predicates (at_kitchen_bread ?b - bread-portion)
	     (at_kitchen_content ?c - content-portion)
         (at_kitchen_sandwich ?s - sandwich)
         (descr ?c - content-portion ?d - content-description)
         (sandwich_contents ?s - sandwich ?d0 ?d1 - content-description)
         (ontray ?s - sandwich ?t - tray)
         (likes ?c - child ?d - content-description)
	     (served ?c - child)
	     (waiting ?c - child ?p - place)
         (at ?t - tray ?p - place)
	     (notexist ?s - sandwich)
  )

(:action make_sandwich
	 :parameters (?s - sandwich ?b - bread-portion ?c0 ?c1 - content-portion ?d0 ?d1 - content-description)
	 :precondition (and (at_kitchen_bread ?b)
			    (at_kitchen_content ?c0)
			    (at_kitchen_content ?c1)
                (not (= ?d0 ?d1))
                (descr ?c0 ?d0)
                (descr ?c1 ?d1)
			    (notexist ?s))
	 :effect (and
		   (not (at_kitchen_bread ?b))
		   (not (at_kitchen_content ?c0))
		   (not (at_kitchen_content ?c1))

		   (at_kitchen_sandwich ?s)
           (sandwich_contents ?s ?d0 ?d1)
		   (not (notexist ?s))
		   ))


(:action put_on_tray
	 :parameters (?s - sandwich ?t - tray)
	 :precondition (and  (at_kitchen_sandwich ?s)
			     (at ?t kitchen))
	 :effect (and
		   (not (at_kitchen_sandwich ?s))
		   (ontray ?s ?t)))


(:action serve_sandwich
 	:parameters (?s - sandwich ?d0 ?d1 - content-description ?c - child ?t - tray ?p - place)
	:precondition (and
               (sandwich_contents ?s ?d0 ?d1)
		       (ontray ?s ?t)
		       (waiting ?c ?p)
		       (likes ?c ?d0)
		       (likes ?c ?d1)
               (at ?t ?p)
		       )
	:effect (and (not (ontray ?s ?t))
		     (served ?c)))

(:action move_tray
	 :parameters (?t - tray ?p1 ?p2 - place)
	 :precondition (and (at ?t ?p1))
	 :effect (and (not (at ?t ?p1))
		      (at ?t ?p2)))
			    

)

