(define (problem Alkene_P03) (:domain Chemical)
(:objects
b071 - boron
c001 - carbon
c002 - carbon
c003 - carbon
c004 - carbon
c005 - carbon
c006 - carbon
c051 - carbon
c052 - carbon
c053 - carbon
c054 - carbon
c055 - carbon
h058 - hydrogen
h059 - hydrogen
h060 - hydrogen
h166 - hydrogen
h167 - hydrogen
h168 - hydrogen
h169 - hydrogen
h170 - hydrogen
h171 - hydrogen
h172 - hydrogen
h173 - hydrogen
h174 - hydrogen
h175 - hydrogen
h179 - hydrogen
h180 - hydrogen
h183 - hydrogen
h184 - hydrogen
o064 - oxygen
o065 - oxygen
o177 - oxygen
o178 - oxygen
o181 - oxygen
o182 - oxygen
)
(:init
(bond b071 c002)
(bond c002 b071)

(bond b071 c005)
(bond c005 b071)

(bond b071 h060)
(bond h060 b071)

(bond c001 c002)
(bond c002 c001)

(bond c002 c003)
(bond c003 c002)

(bond c004 c005)
(bond c005 c004)

(bond c005 c006)
(bond c006 c005)

(doublebond c051 c052)
(doublebond c052 c051)

(bond c051 h166)
(bond h166 c051)

(bond c051 h167)
(bond h167 c051)

(bond c052 c053)
(bond c053 c052)

(bond c052 h168)
(bond h168 c052)

(bond c053 h170)
(bond h170 c053)

(bond c053 h169)
(bond h169 c053)

(bond c053 c054)
(bond c054 c053)

(bond c054 c055)
(bond c055 c054)

(bond c054 h172)
(bond h172 c054)

(bond c054 h171)
(bond h171 c054)

(bond c055 h175)
(bond h175 c055)

(bond c055 h174)
(bond h174 c055)

(bond c055 h173)
(bond h173 c055)

(bond h058 o064)
(bond o064 h058)

(bond h059 o065)
(bond o065 h059)

(bond h179 o177)
(bond o177 h179)

(bond h180 o178)
(bond o178 h180)

(bond h183 o181)
(bond o181 h183)

(bond h184 o182)
(bond o182 h184)

(bond o064 o065)
(bond o065 o064)

(bond o177 o178)
(bond o178 o177)

(bond o181 o182)
(bond o182 o181)
)
(:goal
(and
(bond c051 c052)
(bond c052 c051)

(bond c051 o065)
(bond o065 c051)

(bond c051 h167)
(bond h167 c051)

(bond c051 h166)
(bond h166 c051)

(bond c052 c053)
(bond c053 c052)

(bond c052 h060)
(bond h060 c052)

(bond c052 h168)
(bond h168 c052)

(bond c053 c054)
(bond c054 c053)

(bond c053 h169)
(bond h169 c053)

(bond c053 h170)
(bond h170 c053)

(bond c054 c055)
(bond c055 c054)

(bond c054 h171)
(bond h171 c054)

(bond c054 h172)
(bond h172 c054)

(bond c055 h173)
(bond h173 c055)

(bond c055 h175)
(bond h175 c055)

(bond c055 h174)
(bond h174 c055)

(bond h059 o065)
(bond o065 h059)
)
)
)