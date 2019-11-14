#! /usr/bin/env python

from __future__ import division

import re
import sys


from lab.parser import Parser

PATTERNS = [
    ('coverage', r'Final value: (\d+)', int)
]

DRIVER_PATTERNS = [
    ('search_time', r'run-search wall-clock time: (.+)s', float),
]

ERR_PATTERNS = [
    ('peak_memory', r'Maximum resident set size \(kbytes\): (\d+)', int),
    ('visited', r'States visited: (\d+)', int),
]


class LRPGParser(Parser):
    def __init__(self):
        # print('Running FS output parser')
        Parser.__init__(self)

        for name, pattern, typ in PATTERNS:
            self.add_pattern(name, pattern, type=typ)

        for name, pattern, typ in DRIVER_PATTERNS:
            self.add_pattern(name, pattern, file='driver.log', type=typ)

        for name, pattern, typ in ERR_PATTERNS:
            self.add_pattern(name, pattern, file='run.err', type=typ)

LRPGParser().parse()
