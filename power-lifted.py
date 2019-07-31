#! /usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import os
import subprocess

def parse_options():
    parser = argparse.ArgumentParser()
    parser.add_argument('--translator-output-file', dest='translator_file',
                        help='Output file for the translator')
    parser.add_argument('--domain', dest='domain', action='store', default=None,
                        help='Domain file in PDDL')
    parser.add_argument('--instance', dest='instance', action='store', default=None,
                        help='instance file in PDDL')
    return parser.parse_args()

if __name__ == '__main__':
    options = parse_options()
    subprocess.check_call(['/home/blaas/thesis/planner/src/translator/translate.py',
                           options.domain, options.instance])
