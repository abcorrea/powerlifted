(define (problem Alkene_P06) (:domain Chemical)
(:objects
c016 - carbon
c017 - carbon
c018 - carbon
c019 - carbon
c020 - carbon
c021 - carbon
c022 - carbon
c023 - carbon
c024 - carbon
c046 - carbon
c047 - carbon
c048 - carbon
c049 - carbon
c050 - carbon
h062 - hydrogen
h106 - hydrogen
h107 - hydrogen
h108 - hydrogen
h109 - hydrogen
h110 - hydrogen
h111 - hydrogen
h112 - hydrogen
h113 - hydrogen
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
o066 - oxygen
o067 - oxygen
o068 - oxygen
o069 - oxygen
o070 - oxygen
s072 - sulfur
)
(:init
(bond c016 h108)
(bond h108 c016)

(bond c016 h107)
(bond h107 c016)

(bond c016 h106)
(bond h106 c016)

(bond c016 o066)
(bond o066 c016)

(bond c017 h111)
(bond h111 c017)

(bond c017 h110)
(bond h110 c017)

(bond c017 h109)
(bond h109 c017)

(bond c017 c018)
(bond c018 c017)

(bond c018 h113)
(bond h113 c018)

(bond c018 o067)
(bond o067 c018)

(bond c018 h112)
(bond h112 c018)

(doublebond c019 c024)
(doublebond c024 c019)

(bond c019 c020)
(bond c020 c019)

(doublebond c020 c021)
(doublebond c021 c020)

(bond c021 s072)
(bond s072 c021)

(bond c021 c022)
(bond c022 c021)

(doublebond c022 c023)
(doublebond c023 c022)

(bond c023 c024)
(bond c024 c023)

(doublebond c046 c047)
(doublebond c047 c046)

(bond c046 h158)
(bond h158 c046)

(bond c046 h157)
(bond h157 c046)

(bond c047 h159)
(bond h159 c047)

(bond c047 c048)
(bond c048 c047)

(bond c048 c049)
(bond c049 c048)

(bond c048 h161)
(bond h161 c048)

(bond c048 h160)
(bond h160 c048)

(bond c049 h163)
(bond h163 c049)

(bond c049 h162)
(bond h162 c049)

(bond c049 c050)
(bond c050 c049)

(bond c050 h165)
(bond h165 c050)

(bond c050 h164)
(bond h164 c050)

(bond c050 o070)
(bond o070 c050)

(bond h062 o067)
(bond o067 h062)

(bond h176 o070)
(bond o070 h176)

(bond o066 s072)
(bond s072 o066)

(doublebond o068 s072)
(doublebond s072 o068)

(doublebond o069 s072)
(doublebond s072 o069)
)
(:goal
(and
(bond c016 o070)
(bond o070 c016)

(bond c016 h107)
(bond h107 c016)

(bond c016 h106)
(bond h106 c016)

(bond c016 h108)
(bond h108 c016)

(bond c017 c018)
(bond c018 c017)

(bond c017 h109)
(bond h109 c017)

(bond c017 h111)
(bond h111 c017)

(bond c017 h110)
(bond h110 c017)

(bond c018 o067)
(bond o067 c018)

(bond c018 h113)
(bond h113 c018)

(bond c018 h112)
(bond h112 c018)

(bond c046 c047)
(bond c047 c046)

(bond c046 h062)
(bond h062 c046)

(bond c046 h158)
(bond h158 c046)

(bond c046 h157)
(bond h157 c046)

(bond c047 c048)
(bond c048 c047)

(bond c047 o067)
(bond o067 c047)

(bond c047 h159)
(bond h159 c047)

(bond c048 c049)
(bond c049 c048)

(bond c048 h160)
(bond h160 c048)

(bond c048 h161)
(bond h161 c048)

(bond c049 c050)
(bond c050 c049)

(bond c049 h162)
(bond h162 c049)

(bond c049 h163)
(bond h163 c049)

(bond c050 o070)
(bond o070 c050)

(bond c050 h164)
(bond h164 c050)

(bond c050 h165)
(bond h165 c050)
)
)
)