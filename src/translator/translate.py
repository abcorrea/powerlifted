#! /usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function

import os
import signal
import sys
import traceback


def python_version_supported():
    major, minor = sys.version_info[:2]
    return (major == 2 and minor >= 7) or (major, minor) >= (3, 2)


if not python_version_supported():
    sys.exit("Error: Translator only supports Python >= 2.7 and Python >= 3.2.")

import compile_types
import complete_state
import normalize
import options
import pddl_parser
import reachability
import static_predicates
import timers

DEBUG = False

## For a full list of exit codes, please see driver/returncodes.py. Here,
## we only list codes that are used by the translator component of the planner.
TRANSLATE_OUT_OF_MEMORY = 20
TRANSLATE_OUT_OF_TIME = 21


def main():
    timer = timers.Timer()
    with timers.timing("Parsing", True):
        task = pddl_parser.open(
            domain_filename=options.domain, task_filename=options.task)
    print('Processing task', task.task_name)
    with timers.timing("Normalizing task"):
        normalize.normalize(task)

    with timers.timing("Compiling types into unary predicates"):
        g = compile_types.compile_types(task)

    with timers.timing("Checking static predicates"):
        static_predicates.check(task)

    with timers.timing("Generating complete initial state"):
        #complete_state.generate_complete_initial_state(task, g)
        reachability.generate_overapproximated_reachable_atoms(task)

    print("Initial state length:", len(task.init))
    # task.dump()


def handle_sigxcpu(signum, stackframe):
    print()
    print("Translator hit the time limit")
    # sys.exit() is not safe to be called from within signal handlers, but
    # os._exit() is.
    os._exit(TRANSLATE_OUT_OF_TIME)


if __name__ == "__main__":
    try:
        signal.signal(signal.SIGXCPU, handle_sigxcpu)
    except AttributeError:
        print("Warning! SIGXCPU is not available on your platform. "
              "This means that the planner cannot be gracefully terminated "
              "when using a time limit, which, however, is probably "
              "supported on your platform anyway.")
    try:
        # Reserve about 10 MB (in Python 2) of emergency memory.
        # https://stackoverflow.com/questions/19469608/
        emergency_memory = "x" * 10 ** 7
        main()
    except MemoryError:
        emergency_memory = ""
        print()
        print("Translator ran out of memory, traceback:")
        print("=" * 79)
        traceback.print_exc(file=sys.stdout)
        print("=" * 79)
        sys.exit(TRANSLATE_OUT_OF_MEMORY)
