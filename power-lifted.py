#! /usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import os
import subprocess
import sys

from distutils.dir_util import copy_tree
from shutil import copyfile

from build import get_build_dir, build, PROJECT_ROOT

def parse_options():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--domain', dest='domain', action='store',
                                  default=None, help='Domain file in PDDL', required=True)
    parser.add_argument('-i', '--instance', dest='instance',
                        action='store', default=None,
                        help='Instance file in PDDL', required=True)
    parser.add_argument('--build', dest='build', action='store_true',
                        help='Build planner before search.')
    parser.add_argument('--debug', dest='debug', action='store_true',
                        help='Run planner in debug mode.')
    parser.add_argument('-s', '--search', dest='search', action='store',
                        default=None, help='Search algorithm',
                        required=True)
    parser.add_argument('-e', '--heuristic', dest='heuristic', action='store',
                        default=None,
                        help='Heuristic to guide the search (ignore in case of blind search)',
                        required=True)
    parser.add_argument('-g', '--generator', dest='generator', action='store',
                        default=None, help='Successor generator method',
                        required=True)
    parser.add_argument('--translator-output-file', dest='translator_file',
                        default='output.lifted',
                        help='Output file of the translator')
    return parser.parse_args()

if __name__ == '__main__':
    options = parse_options()

    BUILD = os.path.join(PROJECT_ROOT, 'builds', 'release')
    if options.debug:
        BUILD = os.path.join(PROJECT_ROOT, 'builds', 'debug')

    if options.build:
        build(options.debug)

    # Create build path
    if not os.path.exists(BUILD):
        raise OSError("Planner not built!")


    os.chdir(PROJECT_ROOT)
    subprocess.check_call([os.path.join(BUILD, 'translator', 'translate.py'),
                           options.domain, options.instance,
                           '--output-file', options.translator_file])
    subprocess.check_call([os.path.join(BUILD, 'search', 'search'),
                           options.translator_file,
                           options.search,
                           options.heuristic,
                           options.generator])
