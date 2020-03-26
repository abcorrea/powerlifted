#! /usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import os
import subprocess

from distutils.dir_util import copy_tree
from shutil import copyfile

def parse_options():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--domain', dest='domain', action='store',
                                  default=None, help='Domain file in PDDL', required=True)
    parser.add_argument('-i', '--instance', dest='instance',
                        action='store', default=None,
                        help='Instance file in PDDL', required=True)
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
    dir_path = os.path.dirname(os.path.realpath(__file__))
    options = parse_options()


    # Create build path
    if not os.path.exists(dir_path+'/builds/release/search'):
        os.makedirs(dir_path+'/builds/release/search')

    copy_tree(dir_path+'/src/translator/', dir_path+'/builds/release/translator')
    os.chdir(dir_path+'/builds/release/search')
    subprocess.check_call(['cmake', dir_path+'/src/search/'])
    subprocess.check_call(['make', '-j6'])
    
    os.chdir(dir_path)
    subprocess.check_call([dir_path+'/builds/release/translator/translate.py',
                           options.domain, options.instance,
                           '--output-file', options.translator_file])
    subprocess.check_call([dir_path+'/builds/release/search/search',
                           options.translator_file,
                           options.search,
                           options.heuristic,
                           options.generator])
