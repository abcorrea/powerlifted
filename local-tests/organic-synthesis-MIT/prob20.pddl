(define (problem initialBonds20) (:domain Chemical)
(:objects
; setup for problem 20 
c1 - carbon
c2 - carbon
c3 - carbon
c4 - carbon
c5 - carbon
c6 - carbon
h1 - hydrogen
h2 - hydrogen
h3 - hydrogen
h4 - hydrogen
h5 - hydrogen
h6 - hydrogen
h7 - hydrogen
h8 - hydrogen
h9 - hydrogen
h10 - hydrogen
; water 
o1 - oxygen
h11 - hydrogen
h12 - hydrogen
; water_2
o50 - oxygen
h50 - hydrogen
h51 - hydrogen
; PBr3 
p1 - phosphorus
br1 - bromine
br2 - bromine
br3 - bromine
; free Mg 
mg1 - magnesium
; free hydrogen
h13 - hydrogen
; CO2 
c7 - carbon
o2 - oxygen
o3 - oxygen
; Thionyl chloride 
su1 - sulfur
o4 - oxygen
cl1 - chlorine
cl2 - chlorine
; MeOH 
c8 - carbon
o5 - oxygen
h14 - hydrogen
h15 - hydrogen
h16 - hydrogen
h17 - hydrogen
; second PBr3 
p2 - phosphorus
br4 - bromine
br5 - bromine
br6 - bromine
; free Mg 
mg2 - magnesium
; free hydrogen
h18 - hydrogen
; Second MeOH 
c9 - carbon
o6 - oxygen
h19 - hydrogen
h20 - hydrogen
h21 - hydrogen
h22 - hydrogen
; third PBr3 
p3 - phosphorus
br7 - bromine
br8 - bromine
br9 - bromine
; free Mg 
mg3 - magnesium
)
(:init
; setup for problem 20 
(doublebond c1 c2)
(doublebond c2 c1)
(bond c2 c3)
(bond c3 c4)
(bond c4 c5)
(bond c5 c6)
(bond c6 c1)
(bond c3 c2)
(bond c4 c3)
(bond c5 c4)
(bond c6 c5)
(bond c1 c6)
(bond h1 c1)
(bond h2 c2)
(bond c1 h1)
(bond c2 h2)
(bond h3 c3)
(bond h4 c3)
(bond h5 c4)
(bond h6 c4)
(bond h7 c5)
(bond h8 c5)
(bond h9 c6)
(bond h10 c6)
(bond c3 h3)
(bond c3 h4)
(bond c4 h5)
(bond c4 h6)
(bond c5 h7)
(bond c5 h8)
(bond c6 h9)
(bond c6 h10)
; water 
(bond h11 o1)
(bond h12 o1)
(bond o1 h11)
(bond o1 h12)
; water_2
(bond h50 o50)
(bond h51 o50)
(bond o50 h50)
(bond o50 h51)
; PBr3 
(bond p1 br1)
(bond p1 br2)
(bond p1 br3)
(bond br1 p1)
(bond br2 p1)
(bond br3 p1)
; free Mg 
; free hydrogen
; CO2 
(doublebond o2 c7)
(doublebond c7 o2)
(doublebond o3 c7)
(doublebond c7 o3)
; Thionyl chloride 
(doublebond su1 o4)
(doublebond o4 su1)
(bond cl1 su1)
(bond cl2 su1)
(bond su1 cl1)
(bond su1 cl2)
; MeOH 
(bond c8 o5)
(bond o5 c8)
(bond c8 h14)
(bond c8 h15)
(bond c8 h16)
(bond h14 c8)
(bond h15 c8)
(bond h16 c8)
(bond h17 o5)
(bond o5 h17)
; second PBr3 
(bond p2 br4)
(bond p2 br5)
(bond p2 br6)
(bond br4 p2)
(bond br5 p2)
(bond br6 p2)
; free Mg 
; free hydrogen
; Second MeOH 
(bond c9 o6)
(bond o6 c9)
(bond c9 h19)
(bond c9 h20)
(bond c9 h21)
(bond h19 c9)
(bond h20 c9)
(bond h21 c9)
(bond h22 o6)
(bond o6 h22)
; third PBr3 
(bond p3 br7)
(bond p3 br8)
(bond p3 br9)
(bond br7 p3)
(bond br8 p3)
(bond br9 p3)
; free Mg 
)
(:goal
(and
(bond c2 c1)
(bond c1 c6)
(bond c6 c5)
(bond c5 c4)
(bond c4 c3)
(bond c3 c2)
(bond c2 c7)
(bond c7 o3)
(bond c7 c8)
(bond c7 c9)
(bond c2 h2)
(bond c1 h1)
(bond c1 h13)
(bond c6 h9)
(bond c6 h10)
(bond c5 h7)
(bond c5 h8)
(bond c4 h5)
(bond c4 h6)
(bond c3 h3)
(bond c3 h4)
(bond o3 h50)
(bond c8 h14)
(bond c8 h15)
(bond c8 h16)
(bond c9 h19)
(bond c9 h20)
(bond c9 h21)
)
)
)
