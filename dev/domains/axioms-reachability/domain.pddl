;; Graph reachability with a recursive derived predicate.
;;
;; Roads can be built along the (static) edges of a graph. The derived
;; predicate (reachable ?x ?y) is the transitive closure of the fluent
;; (road ?x ?y): the recursive rule has the existentially quantified body
;; variable ?z.

(define (domain axioms-reachability)
  (:requirements :strips :typing :derived-predicates)
  (:types node - object)
  (:predicates
    (edge ?x ?y - node)
    (road ?x ?y - node)
    (reachable ?x ?y - node))

  (:derived (reachable ?x ?y - node)
    (road ?x ?y))

  (:derived (reachable ?x ?y - node)
    (exists (?z - node) (and (road ?x ?z) (reachable ?z ?y))))

  (:action build-road
    :parameters (?x ?y - node)
    :precondition (edge ?x ?y)
    :effect (road ?x ?y))
)
