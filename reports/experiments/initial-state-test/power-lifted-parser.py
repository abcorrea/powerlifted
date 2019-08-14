#! /usr/bin/env python

import re
import sys

from lab.parser import Parser

PATTERNS = [
    ('initial_state_size', r'Initial state size: (\d+)', int),
    ('peak_memory', r'Peak memory usage: (\d+) kB', int),
    ('search_time', r'Goal found at: (.+)', float),
    ('cost', r'Total plan cost:(\d+)', int),
    ('generations', r'Generations: (\d+)', int),
    ('successors', r'Different states: (.+)', float)
]

def add_coverage(content, props):
    props['coverage'] = int('cost' in props)


class PowerLiftedParser(Parser):
    def __init__(self):
        Parser.__init__(self)

        for name, pattern, typ in PATTERNS:
            self.add_pattern(name, pattern, type=typ)
        self.add_function(add_coverage)


def main():
    parser = PowerLiftedParser()
    parser.parse()

main()
