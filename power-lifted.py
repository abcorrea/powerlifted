#! /usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import os
import subprocess

from distutils.dir_util import copy_tree
from shutil import copyfile

def parse_options():
    parser = argparse.ArgumentParser()
    parser.add_argument('--translator-output-file',
                        dest='translator_file',
                        default='output.lifted',
                        help='Output file of the translator')
    parser.add_argument('--domain', dest='domain', action='store',
                        default=None, help='Domain file in PDDL')
    parser.add_argument('--instance', dest='instance', action='store',
                        default=None, help='Instance file in PDDL')
    return parser.parse_args()

if __name__ == '__main__':
    dir_path = os.path.dirname(os.path.realpath(__file__))
    options = parse_options()
    os.mkdir(dir_path+'/builds')
    os.mkdir(dir_path+'/builds/release')
    os.mkdir(dir_path+'/builds/release/src')
    os.chdir(dir_path+'/builds/release/src')
    copy_tree(dir_path+'/src/translator/', dir_path+'/builds/release/translator')
    subprocess.check_call(['cmake', dir_path+'/src/search/'])
    subprocess.check_call(['make'])
    os.chdir(dir_path)
    subprocess.check_call(['./src/translator/translate.py',
                           options.domain, options.instance,
                           '--output-file', options.translator_file])
