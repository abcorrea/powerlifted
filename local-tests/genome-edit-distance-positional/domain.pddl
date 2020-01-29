
;; Positional single-step domain formulation, ITT operations only.

(define (domain genome-edit-distance)
  (:requirements :typing :adl :action-costs)

  ;; for VAL:
  ;; (:requirements :typing :adl :fluents)

  (:types gene pos)

  (:predicates
   ;; Static predicates describing relations between positions:

   ;; ?y is next (clockwise) from ?x
   (cw ?x - pos ?y - pos)

   ;; ?z is between ?x and ?y, inclusive
   (between ?x - pos ?y - pos ?z - pos)

   ;; transposing [?x,?y] to after ?z moves ?w to ?w-new
   (transpose-shift ?x ?y ?z ?w ?w-new - pos)

   ;; inverting [?x,?y] moves ?w to ?w-new
   (invert-shift ?x ?y ?w ?w-new - pos)

   ;; transverting [?x,?y] to after ?z moves ?w to ?w-new
   (transvert-shift ?x ?y ?z ?w ?w-new - pos)

   ;; Predicates describing the genome:
   (at ?g - gene ?x - pos)
   (normal ?g - gene)
   (inverted ?g - gene)

   ;; dummy control predicate
   (idle)
   )

  (:functions
   (total-cost)
   )


  ;; Actions

  (:action transpose
   :parameters (?x - pos ?y - pos ?z - pos)
   :precondition (and (not (between ?x ?y ?z))
		      (not (cw ?z ?x)))
   :effect (and (forall (?w - pos ?w-new - pos ?g - gene)
			(when (and (transpose-shift ?x ?y ?z ?w ?w-new)
				   (at ?g ?w))
			  (and (not (at ?g ?w))
			       (at ?g ?w-new))))
		(increase (total-cost) 2)
		)
   )

  (:action invert
   :parameters (?x - pos ?y - pos)
   :precondition (and (idle))
   :effect (and (forall (?z - pos ?z-new - pos ?g - gene)
			(when (and (invert-shift ?x ?y ?z ?z-new)
				   (at ?g ?z))
			  (and (not (at ?g ?z))
			       (at ?g ?z-new))))		
		(forall (?z - pos ?g - gene)
			(when (and (between ?x ?y ?z)
				   (at ?g ?z)
				   (normal ?g))
			  (and (not (normal ?g))
			       (inverted ?g))))
		(forall (?z - pos ?g - gene)
			(when (and (between ?x ?y ?z)
				   (at ?g ?z)
				   (inverted ?g))
			  (and (not (inverted ?g))
			       (normal ?g))))
		(increase (total-cost) 1)
		)
   )

  (:action transvert
   :parameters (?x - pos ?y - pos ?z - pos)
   :precondition (and (not (between ?x ?y ?z))
		      (not (cw ?z ?x)))
   :effect (and (forall (?w - pos ?w-new - pos ?g - gene)
			(when (and (transvert-shift ?x ?y ?z ?w ?w-new)
				   (at ?g ?w))
			  (and (not (at ?g ?w))
			       (at ?g ?w-new))))		
		(forall (?w - pos ?g - gene)
			(when (and (between ?x ?y ?w)
				   (at ?g ?w)
				   (normal ?g))
			  (and (not (normal ?g))
			       (inverted ?g))))
		(forall (?w - pos ?g - gene)
			(when (and (between ?x ?y ?w)
				   (at ?g ?w)
				   (inverted ?g))
			  (and (not (inverted ?g))
			       (normal ?g))))
		(increase (total-cost) 2)
		)
   )

  ;; The rotate action moves every gene one step clockwise. This action
  ;; has zero cost.

  (:action rotate
   :parameters ()
   :precondition (and (idle))
   :effect (and (forall (?z - pos ?z-post - pos ?g - gene)
			(when (and (cw ?z ?z-post)
				   (at ?g ?z))
			  (and (not (at ?g ?z))
			       (at ?g ?z-post)))))
   )

  )
