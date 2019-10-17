(define (problem MIT_P09) (:domain Chemical)
(:objects
c1 - carbon
c108 - carbon
c109 - carbon
c11 - carbon
c12 - carbon
c13 - carbon
c2 - carbon
c3 - carbon
c4 - carbon
c5 - carbon
c6 - carbon
c7 - carbon
h1 - hydrogen
h2 - hydrogen
h28 - hydrogen
h29 - hydrogen
h3 - hydrogen
h30 - hydrogen
h32 - hydrogen
h33 - hydrogen
h34 - hydrogen
h35 - hydrogen
h36 - hydrogen
h37 - hydrogen
h4 - hydrogen
h5 - hydrogen
h50 - hydrogen
h51 - hydrogen
i1 - iodine
i2 - iodine
o1 - oxygen
o102 - oxygen
o4 - oxygen
)
(:init
(bond c1 h1)
(bond h1 c1)

(aromaticbond c1 c2)
(aromaticbond c2 c1)

(aromaticbond c1 c6)
(aromaticbond c6 c1)

(bond c108 h50)
(bond h50 c108)

(bond c108 h51)
(bond h51 c108)

(bond c108 c7)
(bond c7 c108)

(bond c108 c109)
(bond c109 c108)

(bond c109 o4)
(bond o4 c109)

(doublebond c109 o102)
(doublebond o102 c109)

(bond c11 h28)
(bond h28 c11)

(bond c11 h29)
(bond h29 c11)

(bond c11 h30)
(bond h30 c11)

(bond c11 o4)
(bond o4 c11)

(bond c12 h32)
(bond h32 c12)

(bond c12 h33)
(bond h33 c12)

(bond c12 h34)
(bond h34 c12)

(bond c12 i1)
(bond i1 c12)

(bond c13 h35)
(bond h35 c13)

(bond c13 h36)
(bond h36 c13)

(bond c13 h37)
(bond h37 c13)

(bond c13 i2)
(bond i2 c13)

(bond c2 h2)
(bond h2 c2)

(aromaticbond c2 c3)
(aromaticbond c3 c2)

(bond c3 h3)
(bond h3 c3)

(aromaticbond c3 c4)
(aromaticbond c4 c3)

(bond c4 h4)
(bond h4 c4)

(aromaticbond c4 c5)
(aromaticbond c5 c4)

(bond c5 h5)
(bond h5 c5)

(aromaticbond c5 c6)
(aromaticbond c6 c5)

(bond c6 c7)
(bond c7 c6)

(doublebond c7 o1)
(doublebond o1 c7)
)
(:goal
(and
(aromaticbond c1 c6)
(aromaticbond c6 c1)

(bond c1 h1)
(bond h1 c1)

(aromaticbond c1 c2)
(aromaticbond c2 c1)

(bond c108 c12)
(bond c12 c108)

(bond c108 c13)
(bond c13 c108)

(bond c108 c7)
(bond c7 c108)

(bond c108 c109)
(bond c109 c108)

(bond c109 o4)
(bond o4 c109)

(doublebond c109 o102)
(doublebond o102 c109)

(bond c11 o4)
(bond o4 c11)

(bond c11 h28)
(bond h28 c11)

(bond c11 h29)
(bond h29 c11)

(bond c11 h30)
(bond h30 c11)

(bond c12 h32)
(bond h32 c12)

(bond c12 h33)
(bond h33 c12)

(bond c12 h34)
(bond h34 c12)

(bond c13 h35)
(bond h35 c13)

(bond c13 h36)
(bond h36 c13)

(bond c13 h37)
(bond h37 c13)

(bond c2 h2)
(bond h2 c2)

(aromaticbond c2 c3)
(aromaticbond c3 c2)

(bond c3 h3)
(bond h3 c3)

(aromaticbond c3 c4)
(aromaticbond c4 c3)

(bond c4 h4)
(bond h4 c4)

(aromaticbond c4 c5)
(aromaticbond c5 c4)

(aromaticbond c5 c6)
(aromaticbond c6 c5)

(bond c5 h5)
(bond h5 c5)

(bond c6 c7)
(bond c7 c6)

(doublebond c7 o1)
(doublebond o1 c7)
)
)
)