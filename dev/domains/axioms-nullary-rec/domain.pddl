;; Mutually recursive *nullary* derived predicates: p and q derive each
;; other (one positive cycle, so they share a stratum) and q also follows
;; from the base fluent. Nullary recursive dependencies have no join-table
;; position, so they exercise the full-re-evaluation trigger of the
;; semi-naive axiom evaluation.

(define (domain axioms-nullary-rec)
  (:requirements :strips :derived-predicates)
  (:predicates
    (base)
    (p)
    (q)
    (done))

  (:derived (p) (q))

  (:derived (q) (p))

  (:derived (q) (base))

  (:action make-base
    :parameters ()
    :precondition (and)
    :effect (base))

  (:action finish
    :parameters ()
    :precondition (p)
    :effect (done))
)
