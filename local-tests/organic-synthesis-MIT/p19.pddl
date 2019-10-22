(define (problem MIT_P19) (:domain Chemical)
(:objects
al2 - aluminium
c1 - carbon
c10 - carbon
c2 - carbon
c3 - carbon
c5 - carbon
c6 - carbon
c7 - carbon
c8 - carbon
c9 - carbon
h1 - hydrogen
h10 - hydrogen
h11 - hydrogen
h12 - hydrogen
h2 - hydrogen
h22 - hydrogen
h23 - hydrogen
h24 - hydrogen
h25 - hydrogen
h3 - hydrogen
h50 - hydrogen
h51 - hydrogen
h52 - hydrogen
h53 - hydrogen
h54 - hydrogen
h8 - hydrogen
h9 - hydrogen
li1 - lithium
n1 - nitrogen
o1 - oxygen
o50 - oxygen
o51 - oxygen
)
(:init
(bond al2 h22)
(bond h22 al2)

(bond al2 h23)
(bond h23 al2)

(bond al2 h24)
(bond h24 al2)

(bond al2 h25)
(bond h25 al2)

(bond c1 h1)
(bond h1 c1)

(bond c1 h2)
(bond h2 c1)

(bond c1 h3)
(bond h3 c1)

(bond c1 c2)
(bond c2 c1)

(bond c10 h12)
(bond h12 c10)

(aromaticbond c10 c5)
(aromaticbond c5 c10)

(aromaticbond c10 c9)
(aromaticbond c9 c10)

(bond c2 c5)
(bond c5 c2)

(doublebond c2 o1)
(doublebond o1 c2)

(bond c3 h54)
(bond h54 c3)

(triplebond c3 n1)
(triplebond n1 c3)

(aromaticbond c5 c6)
(aromaticbond c6 c5)

(bond c6 h8)
(bond h8 c6)

(aromaticbond c6 c7)
(aromaticbond c7 c6)

(bond c7 h9)
(bond h9 c7)

(aromaticbond c7 c8)
(aromaticbond c8 c7)

(bond c8 h10)
(bond h10 c8)

(aromaticbond c8 c9)
(aromaticbond c9 c8)

(bond c9 h11)
(bond h11 c9)

(bond h50 o50)
(bond o50 h50)

(bond h51 o50)
(bond o50 h51)

(bond h52 o51)
(bond o51 h52)

(bond h53 o51)
(bond o51 h53)
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

(aromaticbond c10 c5)
(aromaticbond c5 c10)

(aromaticbond c10 c9)
(aromaticbond c9 c10)

(bond c10 h12)
(bond h12 c10)

(bond c2 c3)
(bond c3 c2)

(bond c2 c5)
(bond c5 c2)

(bond c2 o1)
(bond o1 c2)

(bond c3 h22)
(bond h22 c3)

(bond c3 h23)
(bond h23 c3)

(bond c3 n1)
(bond n1 c3)

(aromaticbond c5 c6)
(aromaticbond c6 c5)

(bond c6 h8)
(bond h8 c6)

(aromaticbond c6 c7)
(aromaticbond c7 c6)

(bond c7 h9)
(bond h9 c7)

(aromaticbond c7 c8)
(aromaticbond c8 c7)

(bond c8 h10)
(bond h10 c8)

(aromaticbond c8 c9)
(aromaticbond c9 c8)

(bond c9 h11)
(bond h11 c9)

(bond h50 n1)
(bond n1 h50)

(bond h52 n1)
(bond n1 h52)

(bond h54 o1)
(bond o1 h54)
)
)
)