#! /usr/bin/env python

import re

from lab.parser import Parser



parser = Parser()
parser.add_pattern(
    'initial_state_size', r'Initial state size: (\d+)')
parser.add_pattern(
    'peak_memory', r'Peak memory usage: (\d+) kB')
parser.add_pattern(
    'plan_cost', r'Total plan cost:(\d+)')
parser.parse()
