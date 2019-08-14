(define (domain blocksworld)

(:types block)

(:predicates (clear ?x)
             (ontable ?x)
             (on ?x ?y))

(:action move-b-to-b
  :parameters (?bm ?bf ?bt)
  :precondition (and (clear ?bm) (clear ?bt) (on ?bm ?bf))
  :effect (and (not (clear ?bt)) (not (on ?bm ?bf))
               (on ?bm ?bt) (clear ?bf)))

(:action move-b-to-t
  :parameters (?bm ?bf)
  :precondition (and (clear ?bm) (on ?bm ?bf))
  :effect (and (not (on ?bm ?bf))
               (ontable ?bm) (clear ?bf)))

(:action move-t-to-b
  :parameters (?bm ?bt)
  :precondition (and (clear ?bm) (clear ?bt) (ontable ?bm))
  :effect (and (not (clear ?bt)) (not (ontable ?bm))
               (on ?bm ?bt))))

