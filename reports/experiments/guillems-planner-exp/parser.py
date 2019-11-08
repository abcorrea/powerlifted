#! /usr/bin/env python
"""
Regular expressions and methods for parsing the output of FS experiments.
"""

from __future__ import division

import re
import sys


from lab.parser import Parser

PATTERNS = [
    ('peak_memory', r'Peak mem. usage: (\d+) kB.', int),
    ('search_time', r'Total Planning Time: (.+) s', float),
    ('generated', r'Generations: (\d+)', int),
    ('expansions', r'Expansions: (\d+)', int),
    ('coverage', r'Search Result: Found plan of length (\d+)', int)
]


class FSOutputParser(Parser):
    def __init__(self):
        # print('Running FS output parser')
        Parser.__init__(self)

        for name, pattern, typ in PATTERNS:
            self.add_pattern(name, pattern, type=typ)


FSOutputParser().parse()
