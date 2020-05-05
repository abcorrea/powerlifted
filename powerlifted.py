#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import os
import subprocess


from build import build, PROJECT_ROOT


def parse_options():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--domain', dest='domain', action='store',
                                  default=None, help='Domain file in PDDL')
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
    parser.add_argument('--state', action='store', help='Successor generator method',
                        default="sparse", choices=("sparse", "extensional"))
    parser.add_argument('--seed', action='store', help='Random seed.',
                        default=1)
    parser.add_argument('--translator-output-file', dest='translator_file',
                        default='output.lifted',
                        help='Output file of the translator')

    args = parser.parse_args()
    if args.domain is None:
        args.domain = find_domain_filename(args.instance)
        if args.domain is None:
            raise RuntimeError('Could not find domain filename that matches instance file ', args.domain)

    return args


def find_domain_filename(task_filename):
    """
    Find domain filename for the given task using automatic naming rules.
    """
    dirname, basename = os.path.split(task_filename)

    domain_basenames = [
        "domain.pddl",
        basename[:3] + "-domain.pddl",
        "domain_" + basename,
        "domain-" + basename,
    ]

    for domain_basename in domain_basenames:
        domain_filename = os.path.join(dirname, domain_basename)
        if os.path.exists(domain_filename):
            return domain_filename


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

    cmd = [os.path.join(BUILD, 'search', 'search'),
           '-f', options.translator_file,
           '-s', options.search,
           '-e', options.heuristic,
           '-g', options.generator,
           '-r', options.state,
           '--seed', str(options.seed)]
    print(f'Executing "{" ".join(cmd)}"')
    subprocess.check_call(cmd)
