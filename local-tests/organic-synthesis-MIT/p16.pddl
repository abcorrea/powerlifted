(define (problem MIT_P16) (:domain Chemical)
(:objects
c1 - carbon
c10 - carbon
c11 - carbon
c12 - carbon
c13 - carbon
c2 - carbon
c3 - carbon
c4 - carbon
c5 - carbon
c6 - carbon
c7 - carbon
c8 - carbon
c9 - carbon
h1 - hydrogen
h12 - hydrogen
h19 - hydrogen
h2 - hydrogen
h20 - hydrogen
h21 - hydrogen
h22 - hydrogen
h23 - hydrogen
h24 - hydrogen
h3 - hydrogen
h4 - hydrogen
h5 - hydrogen
h51 - hydrogen
h6 - hydrogen
o2 - oxygen
o3 - oxygen
o4 - oxygen
o5 - oxygen
o50 - oxygen
o6 - oxygen
)
(:init
(bond c1 h1)
(bond h1 c1)

(bond c1 h2)
(bond h2 c1)

(bond c1 h3)
(bond h3 c1)

(bond c1 c2)
(bond c2 c1)

(bond c10 c9)
(bond c9 c10)

(bond c11 h20)
(bond h20 c11)

(bond c11 h21)
(bond h21 c11)

(bond c11 c2)
(bond c2 c11)

(bond c11 c12)
(bond c12 c11)

(bond c12 c13)
(bond c13 c12)

(doublebond c12 o6)
(doublebond o6 c12)

(bond c13 h22)
(bond h22 c13)

(bond c13 h23)
(bond h23 c13)

(bond c13 h24)
(bond h24 c13)

(bond c2 c7)
(bond c7 c2)

(bond c2 c3)
(bond c3 c2)

(bond c3 h4)
(bond h4 c3)

(bond c3 h5)
(bond h5 c3)

(bond c3 h6)
(bond h6 c3)

(bond c4 c5)
(bond c5 c4)

(bond c5 o2)
(bond o2 c5)

(bond c6 o2)
(bond o2 c6)

(bond c6 c7)
(bond c7 c6)

(doublebond c6 o3)
(doublebond o3 c6)

(bond c7 h19)
(bond h19 c7)

(bond c7 c8)
(bond c8 c7)

(bond c8 o5)
(bond o5 c8)

(doublebond c8 o4)
(doublebond o4 c8)

(bond c9 o5)
(bond o5 c9)

(bond h12 o50)
(bond o50 h12)

(bond h51 o50)
(bond o50 h51)
)
(:goal
(and
(bond c1 c2)
(bond c2 c1)

(bond c1 h1)
(bond h1 c1)

(bond c1 h2)
(bond h2 c1)

(bond c1 h3)
(bond h3 c1)

(bond c11 c2)
(bond c2 c11)

(bond c11 c12)
(bond c12 c11)

(bond c11 h20)
(bond h20 c11)

(bond c11 h21)
(bond h21 c11)

(bond c12 c13)
(bond c13 c12)

(doublebond c12 o6)
(doublebond o6 c12)

(bond c13 c8)
(bond c8 c13)

(bond c13 h23)
(bond h23 c13)

(bond c13 h24)
(bond h24 c13)

(bond c2 c7)
(bond c7 c2)

(bond c2 c3)
(bond c3 c2)

(bond c3 h4)
(bond h4 c3)

(bond c3 h5)
(bond h5 c3)

(bond c3 h6)
(bond h6 c3)

(bond c7 c8)
(bond c8 c7)

(bond c7 h51)
(bond h51 c7)

(bond c7 h19)
(bond h19 c7)

(doublebond c8 o4)
(doublebond o4 c8)
)
)
)