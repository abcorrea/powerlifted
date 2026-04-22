(define (domain object-creation-smoke)
  (:requirements :strips :typing)
  (:types item)
  (:predicates
    (ready)
    (created)
    (owns ?x - item))

  (:action create-item
    :parameters ()
    :precondition (ready)
    :effect (and
      (not (ready))
      (created)
      (:new (?o - item)
        (owns ?o)))))
