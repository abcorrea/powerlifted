; The variants of the Organic Synthesis domain were created by
; Dr. Russell Viirre, Hadi Qovaizi, and Prof. Mikhail Soutchanski.
;
; This work is licensed under a Creative Commons Attribution,
; NonCommercial, ShareAlike 3.0 Unported License.
;
; For further information, please access the following web page:
; https://www.cs.ryerson.ca/~mes/publications/
(define (problem Alkene_P08) (:domain Chemical)
(:objects
c032 - carbon
c033 - carbon
c034 - carbon
c035 - carbon
c036 - carbon
c037 - carbon
c038 - carbon
h056 - hydrogen
h057 - hydrogen
h133 - hydrogen
h134 - hydrogen
h135 - hydrogen
h136 - hydrogen
h137 - hydrogen
h138 - hydrogen
h139 - hydrogen
h140 - hydrogen
h141 - hydrogen
h142 - hydrogen
h143 - hydrogen
h144 - hydrogen
o063 - oxygen
)
(:init
(doublebond c032 c033)
(doublebond c033 c032)

(bond c032 h133)
(bond h133 c032)

(bond c032 h134)
(bond h134 c032)

(bond c033 c038)
(bond c038 c033)

(bond c033 c034)
(bond c034 c033)

(bond c034 h135)
(bond h135 c034)

(bond c034 h136)
(bond h136 c034)

(bond c034 c035)
(bond c035 c034)

(bond c035 h138)
(bond h138 c035)

(bond c035 h137)
(bond h137 c035)

(bond c035 c036)
(bond c036 c035)

(bond c036 h140)
(bond h140 c036)

(bond c036 h139)
(bond h139 c036)

(bond c036 c037)
(bond c037 c036)

(bond c037 h142)
(bond h142 c037)

(bond c037 h141)
(bond h141 c037)

(bond c037 c038)
(bond c038 c037)

(bond c038 h144)
(bond h144 c038)

(bond c038 h143)
(bond h143 c038)

(bond h056 o063)
(bond o063 h056)

(bond h057 o063)
(bond o063 h057)
)
(:goal
(and
(bond c032 c033)
(bond c033 c032)

(bond c032 h134)
(bond h134 c032)

(bond c032 h057)
(bond h057 c032)

(bond c032 h133)
(bond h133 c032)

(bond c033 c034)
(bond c034 c033)

(bond c033 c038)
(bond c038 c033)

(bond c033 o063)
(bond o063 c033)

(bond c034 c035)
(bond c035 c034)

(bond c034 h136)
(bond h136 c034)

(bond c034 h135)
(bond h135 c034)

(bond c035 c036)
(bond c036 c035)

(bond c035 h138)
(bond h138 c035)

(bond c035 h137)
(bond h137 c035)

(bond c036 c037)
(bond c037 c036)

(bond c036 h139)
(bond h139 c036)

(bond c036 h140)
(bond h140 c036)

(bond c037 c038)
(bond c038 c037)

(bond c037 h142)
(bond h142 c037)

(bond c037 h141)
(bond h141 c037)

(bond c038 h144)
(bond h144 c038)

(bond c038 h143)
(bond h143 c038)

(bond h056 o063)
(bond o063 h056)
)
)
)