
;; Relational multi-step domain formulation, with ITT operations only.

(define (domain genome-edit-distance)

  (:predicates
   ;; Static predicate, identifies duplicate copies of genes.
   ;; The "duplicate" relation is symmetric; "swappable" is an
   ;; asymmetric subrelation (used to eliminate symmetric
   ;; instantiations of the swapping operator).
   ;;
   ;; Note: These predicates are not used in the domain version
   ;; with ITT operations only. They are declared only for
   ;; interoperability with problem files that use them.
   (duplicate ?x ?y)
   (swappable ?x ?y)

   ;; Genome representation: The genome is a cycle, represented
   ;; by the relation cw ("clockwise"). Each gene in the genome
   ;; is either "normal" or "inverted". Genes that are not in
   ;; the genome but may be inserted are "free" (and neither
   ;; normal nor inverted), while genes that have been deleted
   ;; are "gone". The predicate "present" is maintained as an
   ;; abbreviation for (and (not (free ?x)) (not (gone ?x))).
   ;;
   ;; Note: Since this domain version implements only the ITT
   ;; operations (i.e., no insertions or deletions), all genes are
   ;; present from the beginning and will remain so in every
   ;; reachable state.
   (cw ?x ?y)
   (free ?x)
   (gone ?x)
   (present ?x)
   (normal ?x)
   (inverted ?x)

   ;; Operation sequencing: The basic edit operations - transpose,
   ;; invert, transvert, delete and insert - have complex "global"
   ;; preconditions and effects. To keep the domain in STRIPS form,
   ;; each of these is broken up into a sequence of steps; the
   ;; sequencing predicates control the structure of a plan, so that
   ;; the steps can only be done in sequences that form valid edit
   ;; operations. Exactly one of the control predicates will be true
   ;; in any reachable state.
   ;;
   ;; There are three basic operations: cutting (cuts a section out
   ;; of the genome, and "stores" it using auxiliary predicates),
   ;; splicing (inserts the cut-out section at some point in the
   ;; genome) and inverse splicing (inserts the cut-out section
   ;; backwards, inverting each gene in it). The genome edit
   ;; operations are implemented using these as follows:
   ;; transpose = cut + splice;
   ;; invert = cut + inverse-splice, where the splice is restricted
   ;;  to be at the same point as the cut;
   ;; transverse = cut + inverse-splice;
   (idle)
   (cutting)
   (have-cut)
   (splicing)
   (inverse-splicing)
   (finished)

   ;; Auxiliary predicates: These are used to remember various bits
   ;; of information during the execution of an edit operation.
   (cut-point-1 ?x)
   (splice-point-1 ?x)
   (splice-point-2 ?x)
   (last-cut-point ?x)
   (s-first ?x)
   (s-next ?x ?y)
   (s-last ?x)
   )

  (:functions
   (total-cost)
   )

  ;; Cutting.
  ;; The cutting operation cuts a variable-length contiguous segment
  ;; out of the genome. The cut segment is "stored" using predicates
  ;; s-first, s-next and s-last (with obvious meanings). A valid
  ;; cutting operation is a sequence of actions of the form
  ;;
  ;;  begin-cut (continue-cut)* end-cut
  ;;
  ;; At the end of the operation, the control predicate have-cut is
  ;; true, and the gene in the genome just before the first cut-out
  ;; gene is marked as last-cut-point.

  (:action begin-cut
   :parameters (?x ?y)
   :precondition (and (not (= ?x ?y))
		      (idle)
		      (cw ?x ?y))
   :effect (and (not (idle))
		(cutting)
		(not (cw ?x ?y))
		(last-cut-point ?x)
		(cut-point-1 ?x)
		(s-first ?y)
		(s-last ?y))
   )

  (:action continue-cut
   :parameters (?x ?y)
   :precondition (and (not (= ?x ?y))
		      (cutting)
		      (s-last ?x)
		      (cw ?x ?y))
   :effect (and (not (cw ?x ?y))
		(not (s-last ?x))
		(s-next ?x ?y)
		(s-last ?y))
   )

  (:action end-cut
   :parameters (?x ?y ?z)
   :precondition (and (not (= ?x ?y))
		      (not (= ?x ?z))
		      (cutting)
		      (s-last ?x)
		      (cw ?x ?y)
		      (cut-point-1 ?z))
   :effect (and (not (cutting))
		(not (cw ?x ?y))
		(not (cut-point-1 ?z))
		(have-cut)
		(cw ?z ?y))
   )


  ;; Splicing
  ;;
  ;; The splicing operation inserts the segment stored by predicates
  ;; s-first, s-next and s-last into the genome, between ?x and ?y.
  ;; A valid transposition splice is a sequence of actions of the form
  ;;
  ;;  begin-transpose-splice (continue-splice)* end-splice
  ;;
  ;; Splicing can only start after a cut, i.e., when (have-cut) is true.

  (:action begin-transpose-splice
   :parameters (?x ?y)
   :precondition (and (not (= ?x ?y))
		      (have-cut)
		      (cw ?x ?y))
   :effect (and (not (have-cut))
		(not (cw ?x ?y))
		(splicing)
		(splice-point-1 ?x)
		(splice-point-2 ?y)
		(increase (total-cost) 2))
   )

  (:action continue-splice
   :parameters (?x ?y ?z)
   :precondition (and (not (= ?x ?y))
		      (not (= ?x ?z))
		      (not (= ?z ?y))
		      (splicing)
		      (s-first ?x)
		      (s-next ?x ?y)
		      (splice-point-1 ?z))
   :effect (and (not (s-first ?x))
		(not (s-next ?x ?y))
		(not (splice-point-1 ?z))
		(s-first ?y)
		(cw ?z ?x)
		(splice-point-1 ?x))
   )

  (:action end-splice
   :parameters (?x ?y ?z)
   :precondition (and (not (= ?x ?y))
		      (not (= ?x ?z))
		      (not (= ?z ?y))
		      (splicing)
		      (s-first ?x)
		      (s-last ?x)
		      (splice-point-1 ?y)
		      (splice-point-2 ?z))
   :effect (and (not (splicing))
		(not (s-first ?x))
		(not (s-last ?x))
		(not (splice-point-1 ?y))
		(not (splice-point-2 ?z))
		(finished)
		(cw ?y ?x)
		(cw ?x ?z))
   )

  ;; Inverse splicing.
  ;;
  ;; The inverse splicing operation inserts the segment stored by s-first,
  ;; s-next and s-last into the genome, reversed, between ?x and ?y, and
  ;; also flips the orientation of each gene in the segment, meaning that
  ;; a gene that is "normal" becomes "inverted" and one that is "inverted"
  ;; becomes "normal".
  ;;
  ;; A valid inverse splice is a sequence of actions of the form
  ;;
  ;;  begin-transverse-splice|begin-inverse-splice
  ;;   (continue-inverse-splice-A|-B)* end-inverse-splice-A|-B
  ;;
  ;; The only difference between begin-transverse-splice (transversion)
  ;; and begin-inverse-splice (inversion) is that the former can choose
  ;; any splice point, while the latter can only splice at last-cut-point.
  ;; The actions have different costs, to reflect the relative weight of
  ;; the two operations.
  ;;
  ;; Inverse splicing can only start after a cut, i.e., when (have-cut) is
  ;; true.

  (:action begin-transverse-splice
   :parameters (?x ?y)
   :precondition (and (not (= ?x ?y))
		      (have-cut)
		      (cw ?x ?y))
   :effect (and (not (have-cut))
		(not (cw ?x ?y))
		(inverse-splicing)
		(splice-point-1 ?x)
		(splice-point-2 ?y)
		(increase (total-cost) 2))
   )

  (:action begin-inverse-splice
   :parameters (?x ?y)
   :precondition (and (not (= ?x ?y))
		      (have-cut)
		      (cw ?x ?y)
		      (last-cut-point ?x))
   :effect (and (not (have-cut))
		(not (cw ?x ?y))
		(inverse-splicing)
		(splice-point-1 ?x)
		(splice-point-2 ?y)
		(increase (total-cost) 1))
   )

  ;; special case: the genome consists of one single gene (i.e., all
  ;; but ?x have been cut); in this case, we should not delete (cw ?x ?x)

  (:action begin-inverse-splice-special-case
   :parameters (?x)
   :precondition (and (have-cut)
		      (cw ?x ?x)
		      (last-cut-point ?x))
   :effect (and (not (have-cut))
		(inverse-splicing)
		(splice-point-1 ?x)
		(splice-point-2 ?x)
		(increase (total-cost) 1))
   )

  (:action continue-inverse-splice-A
   :parameters (?x ?y ?z)
   :precondition (and (not (= ?x ?y))
		      (not (= ?x ?z))
		      (not (= ?z ?y))
		      (inverse-splicing)
		      (splice-point-1 ?z)
		      (normal ?x)
		      (s-last ?x)
		      (s-next ?y ?x))
   :effect (and (not (s-last ?x))
		(not (s-next ?y ?x))
		(not (splice-point-1 ?z))
		(not (normal ?x))
		(s-last ?y)
		(inverted ?x)
		(cw ?z ?x)
		(splice-point-1 ?x))
   )

  (:action continue-inverse-splice-B
   :parameters (?x ?y ?z)
   :precondition (and (not (= ?x ?y))
		      (not (= ?x ?z))
		      (not (= ?z ?y))
		      (inverse-splicing)
		      (splice-point-1 ?z)
		      (inverted ?x)
		      (s-last ?x)
		      (s-next ?y ?x))
   :effect (and (not (s-last ?x))
		(not (s-next ?y ?x))
		(not (inverted ?x))
		(not (splice-point-1 ?z))
		(s-last ?y)
		(normal ?x)
		(cw ?z ?x)
		(splice-point-1 ?x))
   )

  (:action end-inverse-splice-A
   :parameters (?x ?y ?z)
   :precondition (and (not (= ?x ?y))
		      (not (= ?x ?z))
		      (inverse-splicing)
		      (normal ?x)
		      (splice-point-1 ?y)
		      (splice-point-2 ?z)
		      (s-first ?x)
		      (s-last ?x))
   :effect (and (not (inverse-splicing))
		(not (splice-point-1 ?y))
		(not (splice-point-2 ?z))
		(not (s-first ?x))
		(not (s-last ?x))
		(not (normal ?x))
		(finished)
		(cw ?y ?x)
		(cw ?x ?z)
		(inverted ?x))
   )

  (:action end-inverse-splice-B
   :parameters (?x ?y ?z)
   :precondition (and (not (= ?x ?y))
		      (not (= ?x ?z))
		      (inverse-splicing)
		      (inverted ?x)
		      (splice-point-1 ?y)
		      (splice-point-2 ?z)
		      (s-first ?x)
		      (s-last ?x))
   :effect (and (not (inverse-splicing))
		(not (splice-point-1 ?y))
		(not (splice-point-2 ?z))
		(not (s-first ?x))
		(not (s-last ?x))
		(not (inverted ?x))
		(finished)
		(cw ?y ?x)
		(cw ?x ?z)
		(normal ?x))
   )

  ;; The reset action must follow every complete edit operation sequence.
  ;; Its only purpose is to "forget" the last-cut-point.

  (:action reset-1
   :parameters (?x)
   :precondition (and (finished)
		      (last-cut-point ?x))
   :effect (and (not (last-cut-point ?x))
		(not (finished))
		(idle))
   )


  ;; Invariants.
  ;;
  ;; Below is a set of invariants (mutex groups) that are valid for
  ;; this domain formulation, written in DKEL syntax. (They are
  ;; commented out because almost no planner can read DKEL.) Together
  ;; with appropriate tools, these can be used to, for example, control
  ;; translation from PDDL to SAS+.

  ;; :invariant
  ;; :name x-inverted
  ;; :vars (?x)
  ;; :set-constraint (exactly-n 1 (normal ?x) (inverted ?x))
  ;; )
  ;; 
  ;; :invariant
  ;; :name cw-next
  ;; :vars (?x)
  ;; :set-constraint (at-most-n 1 (setof :vars (?y) (cw ?x ?y)))
  ;; )
  ;; 
  ;; :invariant
  ;; :name cw-prev
  ;; :vars (?x)
  ;; :set-constraint (at-most-n 1 (setof :vars (?y) (cw ?y ?x)))
  ;; )
  ;; 
  ;; :invariant
  ;; :name x-s-first
  ;; :set-constraint (at-most-n 1 (setof :vars (?x) (s-first ?x)))
  ;; )
  ;; 
  ;; :invariant
  ;; :name x-s-next
  ;; :vars (?x)
  ;; :set-constraint (at-most-n 1 (setof :vars (?y) (s-next ?x ?y)))
  ;; )
  ;; 
  ;; :invariant
  ;; :name x-s-last
  ;; :set-constraint (at-most-n 1 (setof :vars (?x) (s-last ?x)))
  ;; )
  ;; 
  ;; :invariant
  ;; :name x-last-cut
  ;; :set-constraint (at-most-n 1 (idle) (setof :vars (?x) (last-cut-point ?x)))
  ;; )
  ;; 
  ;; :invariant
  ;; :name x-cut-point-1
  ;; :set-constraint (at-most-n 1 (setof :vars (?x) (cut-point-1 ?x)))
  ;; )
  ;; 
  ;; :invariant
  ;; :name x-splice-point-1
  ;; :set-constraint (at-most-n 1 (setof :vars (?x) (splice-point-1 ?x)))
  ;; )
  ;; 
  ;; :invariant
  ;; :name x-splice-point-2
  ;; :set-constraint (at-most-n 1 (setof :vars (?x) (splice-point-2 ?x)))
  ;; )
  ;; 
  ;; ; this invariant is only valid for the ternary domain formulation
  ;; ; with ITT operation set
  ;; :invariant
  ;; :name control
  ;; :set-constraint (exactly-n 1 (idle) (cutting) (have-cut)
  ;; 			      (splicing) (inverse-splicing)
  ;; 			      (finished))
  ;; )

  )
