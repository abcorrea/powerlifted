(define (problem Alkene_P15) (:domain Chemical)
(:objects
c025 - carbon
c026 - carbon
c027 - carbon
c028 - carbon
c029 - carbon
c030 - carbon
c031 - carbon
h056 - hydrogen
h057 - hydrogen
h119 - hydrogen
h120 - hydrogen
h121 - hydrogen
h122 - hydrogen
h123 - hydrogen
h124 - hydrogen
h125 - hydrogen
h126 - hydrogen
h127 - hydrogen
h128 - hydrogen
h129 - hydrogen
h130 - hydrogen
h131 - hydrogen
h132 - hydrogen
o063 - oxygen
)
(:init
(bond c025 h121)
(bond h121 c025)

(bond c025 h119)
(bond h119 c025)

(bond c025 h120)
(bond h120 c025)

(bond c025 c026)
(bond c026 c025)

(bond c026 h122)
(bond h122 c026)

(bond c026 h123)
(bond h123 c026)

(bond c026 c027)
(bond c027 c026)

(bond c027 h124)
(bond h124 c027)

(bond c027 h125)
(bond h125 c027)

(bond c027 c028)
(bond c028 c027)

(bond c028 h126)
(bond h126 c028)

(bond c028 h127)
(bond h127 c028)

(bond c028 c029)
(bond c029 c028)

(doublebond c029 c030)
(doublebond c030 c029)

(bond c029 h128)
(bond h128 c029)

(bond c030 h129)
(bond h129 c030)

(bond c030 c031)
(bond c031 c030)

(bond c031 h132)
(bond h132 c031)

(bond c031 h130)
(bond h130 c031)

(bond c031 h131)
(bond h131 c031)

(bond h056 o063)
(bond o063 h056)

(bond h057 o063)
(bond o063 h057)
)
(:goal
(and
(bond c025 c026)
(bond c026 c025)

(bond c025 h121)
(bond h121 c025)

(bond c025 h119)
(bond h119 c025)

(bond c025 h120)
(bond h120 c025)

(bond c026 c027)
(bond c027 c026)

(bond c026 h123)
(bond h123 c026)

(bond c026 h122)
(bond h122 c026)

(bond c027 c028)
(bond c028 c027)

(bond c027 h125)
(bond h125 c027)

(bond c027 h124)
(bond h124 c027)

(bond c028 c029)
(bond c029 c028)

(bond c028 h127)
(bond h127 c028)

(bond c028 h126)
(bond h126 c028)

(bond c029 c030)
(bond c030 c029)

(bond c029 h057)
(bond h057 c029)

(bond c029 h128)
(bond h128 c029)

(bond c030 o063)
(bond o063 c030)

(bond c030 c031)
(bond c031 c030)

(bond c030 h129)
(bond h129 c030)

(bond c031 h130)
(bond h130 c031)

(bond c031 h132)
(bond h132 c031)

(bond c031 h131)
(bond h131 c031)

(bond h056 o063)
(bond o063 h056)
)
)
)