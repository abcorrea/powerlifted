(define (problem Alkene_P04) (:domain Chemical)
(:objects
c017 - carbon
c018 - carbon
c051 - carbon
c052 - carbon
c053 - carbon
c054 - carbon
c055 - carbon
h062 - hydrogen
h109 - hydrogen
h110 - hydrogen
h111 - hydrogen
h112 - hydrogen
h113 - hydrogen
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
o067 - oxygen
)
(:init
(bond c017 h110)
(bond h110 c017)

(bond c017 h111)
(bond h111 c017)

(bond c017 h109)
(bond h109 c017)

(bond c017 c018)
(bond c018 c017)

(bond c018 o067)
(bond o067 c018)

(bond c018 h113)
(bond h113 c018)

(bond c018 h112)
(bond h112 c018)

(doublebond c051 c052)
(doublebond c052 c051)

(bond c051 h166)
(bond h166 c051)

(bond c051 h167)
(bond h167 c051)

(bond c052 h168)
(bond h168 c052)

(bond c052 c053)
(bond c053 c052)

(bond c053 h169)
(bond h169 c053)

(bond c053 h170)
(bond h170 c053)

(bond c053 c054)
(bond c054 c053)

(bond c054 h171)
(bond h171 c054)

(bond c054 h172)
(bond h172 c054)

(bond c054 c055)
(bond c055 c054)

(bond c055 h175)
(bond h175 c055)

(bond c055 h173)
(bond h173 c055)

(bond c055 h174)
(bond h174 c055)

(bond h062 o067)
(bond o067 h062)
)
(:goal
(and
(bond c017 c018)
(bond c018 c017)

(bond c017 h110)
(bond h110 c017)

(bond c017 h109)
(bond h109 c017)

(bond c017 h111)
(bond h111 c017)

(bond c018 o067)
(bond o067 c018)

(bond c018 h112)
(bond h112 c018)

(bond c018 h113)
(bond h113 c018)

(bond c051 c052)
(bond c052 c051)

(bond c051 h062)
(bond h062 c051)

(bond c051 h166)
(bond h166 c051)

(bond c051 h167)
(bond h167 c051)

(bond c052 c053)
(bond c053 c052)

(bond c052 o067)
(bond o067 c052)

(bond c052 h168)
(bond h168 c052)

(bond c053 c054)
(bond c054 c053)

(bond c053 h170)
(bond h170 c053)

(bond c053 h169)
(bond h169 c053)

(bond c054 c055)
(bond c055 c054)

(bond c054 h172)
(bond h172 c054)

(bond c054 h171)
(bond h171 c054)

(bond c055 h173)
(bond h173 c055)

(bond c055 h175)
(bond h175 c055)

(bond c055 h174)
(bond h174 c055)
)
)
)