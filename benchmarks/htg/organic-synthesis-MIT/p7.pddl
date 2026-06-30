; The variants of the Organic Synthesis domain were created by
; Dr. Russell Viirre, Hadi Qovaizi, and Prof. Mikhail Soutchanski.
;
; This work is licensed under a Creative Commons Attribution,
; NonCommercial, ShareAlike 3.0 Unported License.
;
; For further information, please access the following web page:
; https://www.cs.ryerson.ca/~mes/publications/
(define (problem MIT_P07) (:domain Chemical)
(:objects
li2 - lithium
al2 - aluminium
c2 - carbon
c3 - carbon
c4 - carbon
c5 - carbon
c6 - carbon
c7 - carbon
c8 - carbon
cl2 - chlorine
h17 - hydrogen
h18 - hydrogen
h19 - hydrogen
h20 - hydrogen
h21 - hydrogen
h23 - hydrogen
h24 - hydrogen
h25 - hydrogen
h26 - hydrogen
h27 - hydrogen
h29 - hydrogen
h30 - hydrogen
h31 - hydrogen
h33 - hydrogen
h35 - hydrogen
h36 - hydrogen
h37 - hydrogen
h38 - hydrogen
h5 - hydrogen
h54 - hydrogen
h6 - hydrogen
n1 - nitrogen
o15 - oxygen
o3 - oxygen
)
(:init
(bond al2 h35)
(bond h35 al2)

(bond al2 h36)
(bond h36 al2)

(bond al2 h37)
(bond h37 al2)

(bond al2 h38)
(bond h38 al2)

(bond c2 h5)
(bond h5 c2)

(bond c2 h6)
(bond h6 c2)

(bond c2 c4)
(bond c4 c2)

(bond c2 n1)
(bond n1 c2)

(bond c3 h17)
(bond h17 c3)

(bond c3 h18)
(bond h18 c3)

(bond c3 h19)
(bond h19 c3)

(bond c3 c4)
(bond c4 c3)

(bond c4 h20)
(bond h20 c4)

(bond c4 h21)
(bond h21 c4)

(bond c5 h23)
(bond h23 c5)

(bond c5 h24)
(bond h24 c5)

(bond c5 h25)
(bond h25 c5)

(bond c5 c6)
(bond c6 c5)

(bond c6 cl2)
(bond cl2 c6)

(doublebond c6 o3)
(doublebond o3 c6)

(bond c7 h29)
(bond h29 c7)

(bond c7 h30)
(bond h30 c7)

(bond c7 h31)
(bond h31 c7)

(bond c7 c8)
(bond c8 c7)

(bond c8 h33)
(bond h33 c8)

(doublebond c8 n1)
(doublebond n1 c8)

(bond h26 o15)
(bond o15 h26)

(bond h27 o15)
(bond o15 h27)

(bond h54 n1)
(bond n1 h54)
)
(:goal
(and
(bond c2 n1)
(bond n1 c2)

(bond c2 c4)
(bond c4 c2)

(bond c2 h5)
(bond h5 c2)

(bond c2 h6)
(bond h6 c2)

(bond c3 c4)
(bond c4 c3)

(bond c3 h17)
(bond h17 c3)

(bond c3 h18)
(bond h18 c3)

(bond c3 h19)
(bond h19 c3)

(bond c4 h20)
(bond h20 c4)

(bond c4 h21)
(bond h21 c4)

(bond c5 c6)
(bond c6 c5)

(bond c5 h23)
(bond h23 c5)

(bond c5 h24)
(bond h24 c5)

(bond c5 h25)
(bond h25 c5)

(bond c6 n1)
(bond n1 c6)

(doublebond c6 o3)
(doublebond o3 c6)

(bond c7 c8)
(bond c8 c7)

(bond c7 h29)
(bond h29 c7)

(bond c7 h30)
(bond h30 c7)

(bond c7 h31)
(bond h31 c7)

(bond c8 n1)
(bond n1 c8)

(bond c8 h33)
(bond h33 c8)

(bond c8 h35)
(bond h35 c8)
)
)
)