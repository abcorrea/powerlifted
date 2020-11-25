#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import errno
import logging
import os
import subprocess
import sys
from pathlib import Path

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
                        default=None, help='Search algorithm', choices=("naive", "gbfs", "lazy", "lazy-po", "lazy-prune"),
                        required=True)
    parser.add_argument('-e', '--heuristic', dest='heuristic', action='store',
                        default=None, choices=("blind", "goalcount", "add", "hmax"),
                        help='Heuristic to guide the search (ignore in case of blind search)',
                        required=True)
    parser.add_argument('-g', '--generator', dest='generator', action='store',
                        default=None, help='Successor generator method',
                        choices=('yannakakis', 'join', 'random_join', 'ordered_join', 'inverse_ordered_join', 'full_reducer'),
                        required=True)
    parser.add_argument('--state', action='store', help='Successor generator method',
                        default="sparse", choices=("sparse", "extensional"))
    parser.add_argument('--seed', action='store', help='Random seed.',
                        default=1)
    parser.add_argument('--translator-output-file', dest='translator_file',
                        default='output.lifted',
                        help='Output file of the translator')
    parser.add_argument('--datalog-file', dest='datalog_file',
                        default='model.lp',
                        help='Datalog model for the lifted heuristic.')
    parser.add_argument("--keep-action-predicates", action="store_true",
                        help="flag if the Datalog model should keep action predicates")
    parser.add_argument("--keep-duplicated-rules", action="store_true",
                        help="flag if the Datalog model should keep duplicated auxiliary rules")

    args = parser.parse_args()
    if args.domain is None:
        args.domain = find_domain_filename(args.instance)
        if args.domain is None:
            raise RuntimeError(f'Could not find domain filename matching instance file "{args.instance}"')

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


def validate(domain_name, instance_name, planfile):
    plan = Path(planfile)
    if not plan.is_file():
        logging.info(f'No plan file to validate could be found at "{planfile}"')
        return

    try:
        validate_inputs = ["validate", domain_name, instance_name, planfile]
        _ = subprocess.call(' '.join(validate_inputs), shell=True)
    except OSError as err:
        if err.errno == errno.ENOENT:
            logging.error("Error: 'validate' binary not found. Is it on the PATH?")
            return
        else:
            logging.error(f"Error executing 'validate': {err}")


def main():
    CPP_EXTRA_OPTIONS = []
    PYTHON_EXTRA_OPTIONS = []

    options = parse_options()

    build_dir = os.path.join(PROJECT_ROOT, 'builds', 'debug' if options.debug else 'release')

    if options.build:
        build(options.debug)

    # Create build path
    if not os.path.exists(build_dir):
        raise OSError("Planner not built!")

    os.chdir(PROJECT_ROOT)

    # If it is the lifted heuristic, we need to obtain the Datalog model
    if options.heuristic == 'add' or options.heuristic == 'hmax':
       PYTHON_EXTRA_OPTIONS += ['--build-datalog-model', '--datalog-file', options.datalog_file]
       if options.keep_action_predicates:
           PYTHON_EXTRA_OPTIONS.append('--keep-action-predicates')
       if options.keep_duplicated_rules:
           PYTHON_EXTRA_OPTIONS.append('--keep-duplicated-rules')
       CPP_EXTRA_OPTIONS += ['--datalog-file', options.datalog_file]

    # Invoke the Python preprocessor
    subprocess.call([os.path.join(build_dir, 'translator', 'translate.py'),
                     options.domain, options.instance, '--output-file', options.translator_file] + \
                    PYTHON_EXTRA_OPTIONS)



    # Invoke the C++ search component
    cmd = [os.path.join(build_dir, 'search', 'search'),
           '-f', options.translator_file,
           '-s', options.search,
           '-e', options.heuristic,
           '-g', options.generator,
           '-r', options.state,
           '--seed', str(options.seed)] + \
           CPP_EXTRA_OPTIONS

    print(f'Executing "{" ".join(cmd)}"')
    code = subprocess.call(cmd)

    # If we found a plan, try to validate it
    if code == 0:
        validate(options.domain, options.instance, os.path.join(PROJECT_ROOT, 'sas_plan'))

    return code


if __name__ == '__main__':
    sys.exit(main())
