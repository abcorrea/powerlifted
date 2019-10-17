(define (problem alkene_p05) (:domain Chemical)
(:objects
c046 - carbon
c047 - carbon
c048 - carbon
c049 - carbon
c050 - carbon
h157 - hydrogen
h158 - hydrogen
h159 - hydrogen
h160 - hydrogen
h161 - hydrogen
h162 - hydrogen
h163 - hydrogen
h164 - hydrogen
h165 - hydrogen
h176 - hydrogen
o070 - oxygen
)
(:init
(doublebond c046 c047)
(doublebond c047 c046)

(bond c046 h157)
(bond h157 c046)

(bond c046 h158)
(bond h158 c046)

(bond c047 h159)
(bond h159 c047)

(bond c047 c048)
(bond c048 c047)

(bond c048 h160)
(bond h160 c048)

(bond c048 h161)
(bond h161 c048)

(bond c048 c049)
(bond c049 c048)

(bond c049 h162)
(bond h162 c049)

(bond c049 h163)
(bond h163 c049)

(bond c049 c050)
(bond c050 c049)

(bond c050 o070)
(bond o070 c050)

(bond c050 h164)
(bond h164 c050)

(bond c050 h165)
(bond h165 c050)

(bond h176 o070)
(bond o070 h176)
)
(:goal
(and
(bond c046 c047)
(bond c047 c046)

(bond c046 h158)
(bond h158 c046)

(bond c046 h157)
(bond h157 c046)

(bond c046 h176)
(bond h176 c046)

(bond c047 c048)
(bond c048 c047)

(bond c047 o070)
(bond o070 c047)

(bond c047 h159)
(bond h159 c047)

(bond c048 c049)
(bond c049 c048)

(bond c048 h161)
(bond h161 c048)

(bond c048 h160)
(bond h160 c048)

(bond c049 c050)
(bond c050 c049)

(bond c049 h163)
(bond h163 c049)

(bond c049 h162)
(bond h162 c049)

(bond c050 o070)
(bond o070 c050)

(bond c050 h165)
(bond h165 c050)

(bond c050 h164)
(bond h164 c050)
)
)
)