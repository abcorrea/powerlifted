
(define (problem example-problem)
  (:domain htg-child-snack)
  (:objects
    child0 child1 child2 - child
    bread0 bread1 bread2 - bread-portion
    content-0 content-1 content-2 - content-description
    content-0-0 content-0-1 content-0-2 content-0-3 content-0-4 content-0-5 content-0-6 content-0-7 content-0-8 content-0-9 - content-portion
    content-1-0 content-1-1 content-1-2 content-1-3 content-1-4 content-1-5 content-1-6 content-1-7 content-1-8 content-1-9 content-1-10 - content-portion
    content-2-0 content-2-1 content-2-2 content-2-3 content-2-4 content-2-5 content-2-6 content-2-7 content-2-8 content-2-9 content-2-10 content-2-11 - content-portion
    tray0 tray1 tray2 - tray
    table0 table1 table2 table3 table4 - place
    sandw0 sandw1 sandw2 - sandwich
  )
  (:init
     ;; everythin starts in the kitchen
     (at tray0 kitchen)
     (at tray1 kitchen)
     (at tray2 kitchen)
			    (at_kitchen_bread bread0)
			    (at_kitchen_bread bread1)
			    (at_kitchen_bread bread2)
			    (at_kitchen_content content-0-0)
			    (at_kitchen_content content-0-1)
			    (at_kitchen_content content-0-2)
			    (at_kitchen_content content-0-3)
			    (at_kitchen_content content-0-4)
			    (at_kitchen_content content-0-5)
			    (at_kitchen_content content-0-6)
			    (at_kitchen_content content-0-7)
			    (at_kitchen_content content-0-8)
			    (at_kitchen_content content-0-9)
			    (at_kitchen_content content-1-0)
			    (at_kitchen_content content-1-1)
			    (at_kitchen_content content-1-2)
			    (at_kitchen_content content-1-3)
			    (at_kitchen_content content-1-4)
			    (at_kitchen_content content-1-5)
			    (at_kitchen_content content-1-6)
			    (at_kitchen_content content-1-7)
			    (at_kitchen_content content-1-8)
			    (at_kitchen_content content-1-9)
			    (at_kitchen_content content-1-10)
			    (at_kitchen_content content-2-0)
			    (at_kitchen_content content-2-1)
			    (at_kitchen_content content-2-2)
			    (at_kitchen_content content-2-3)
			    (at_kitchen_content content-2-4)
			    (at_kitchen_content content-2-5)
			    (at_kitchen_content content-2-6)
			    (at_kitchen_content content-2-7)
			    (at_kitchen_content content-2-8)
			    (at_kitchen_content content-2-9)
			    (at_kitchen_content content-2-10)
			    (at_kitchen_content content-2-11) 

     ;; content descriptions
                (descr content-0-0 content-0)
                (descr content-0-1 content-0)
                (descr content-0-2 content-0)
                (descr content-0-3 content-0)
                (descr content-0-4 content-0)
                (descr content-0-5 content-0)
                (descr content-0-6 content-0)
                (descr content-0-7 content-0)
                (descr content-0-8 content-0)
                (descr content-0-9 content-0)
                (descr content-1-0 content-1)
                (descr content-1-1 content-1)
                (descr content-1-2 content-1)
                (descr content-1-3 content-1)
                (descr content-1-4 content-1)
                (descr content-1-5 content-1)
                (descr content-1-6 content-1)
                (descr content-1-7 content-1)
                (descr content-1-8 content-1)
                (descr content-1-9 content-1)
                (descr content-1-10 content-1)
                (descr content-2-0 content-2)
                (descr content-2-1 content-2)
                (descr content-2-2 content-2)
                (descr content-2-3 content-2)
                (descr content-2-4 content-2)
                (descr content-2-5 content-2)
                (descr content-2-6 content-2)
                (descr content-2-7 content-2)
                (descr content-2-8 content-2)
                (descr content-2-9 content-2)
                (descr content-2-10 content-2)
                (descr content-2-11 content-2)
     
     ;; which sandwichs the kids accept
     (likes child2 content-1)
     (likes child1 content-0)
     (likes child1 content-2)
     (likes child0 content-0)
     (likes child0 content-2)
     (likes child2 content-2)
     (likes child2 content-0)
     (likes child1 content-1)
     (likes child0 content-1)

     ;; where children are
     (waiting child0 table0)
     (waiting child1 table1)
     (waiting child2 table2)

     ;;sandwich dummies
     (notexist sandw0)
     (notexist sandw1)
     (notexist sandw2)
  )
  (:goal
    (and
     (served child0)
     (served child1)
     (served child2)
    )
  )
)

