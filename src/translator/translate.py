#! /usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function

import os
import signal
import sys
import traceback

from collections import defaultdict


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
import pddl
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
        # complete_state.generate_complete_initial_state(task, g)
        reachability.generate_overapproximated_reachable_atoms(task, g)

    print("Initial state length:", len(task.init))

    # Preprocess a dict of supertypes for every type from the TypeGraph
    types_dict = defaultdict(set)
    for current_type in g.types:
        t_name = current_type.name
        types_dict[t_name].add(t_name)
        while t_name != 'object':
            t_name = g.edges[t_name]
            types_dict[current_type.name].add(t_name)

    domain = os.path.basename(os.path.dirname(options.domain))
    inst = os.path.basename(options.task)
    # Print task in easy-to-parse format
    #   1. Print domain and instance names
    print("%s:%s" % (domain, inst))

    #   2. Print canary and number of types, followed by type names and their
    #  type indexes
    print("TYPES %d" % len(task.types))
    type_index = {}
    for index, t in enumerate(task.types):
        type_index[t.name] = index
        print("%s %d" % (t.name, index))

    #   3. Print canary and number of predicates, followed by list of
    # predicates.  Each predicate number is followed by three numbers:
    #      - J, its predicate index,
    #      - N, its arity and then N numbers,
    #      - S, a boolean value indicating if the predicate is static or not
    #
    # After each predicate line, there is a line containing N tuples. Each
    # pair is in the format 'X A', where X can be 'e' or 's', indicating that
    #  the argument is either-typed or single-typed. If it is either typed,
    # it is followed by a pair of type indices. Otherwise, it is followed by
    # a single number, representing the type index.
    print("PREDICATES %d" % len(task.predicates))
    predicate_index = {}
    for index, p in enumerate(task.predicates):
        predicate_index[p.name] = index
        print("%s %d %d %d" % (p.name, index, len(p.arguments), p.static))
        if not p.static:
            # If p is fluent, then we care about the types of its parameters
            args = []
            for arg in p.arguments:
                if not isinstance(arg.type_name, list):
                    args += ['s', str(type_index[arg.type_name])]
                else:
                    # If the predicate is either-typed, add flag 'e' and skip
                    #  reserved word 'either'
                    args += ['e'] + [str(type_index[a]) for a in
                                     arg.type_name[1:]]
            print(' '.join(args))
        else:
            # If it is static, we can assume that all its parameters are of
            # type object, since we cannot generate new atoms of this predicate
            # we cannot mess up it with static predicates. (Assuming everything
            # done before is correct.
            print(
                ' '.join(str(type_index['object']) for arg in p.arguments))

    #   4. Print a canary and the number of objects, followed by a list of
    # objects.  Each object has a name, an index, and the number of
    # types/supertypes.  It is then followed by a line containing every
    # supertype and type that the object belongs to.
    print("OBJECTS %d" % len(task.objects))
    object_index = {}
    for index, obj in enumerate(task.objects):
        object_index[obj.name] = index
        print('%s %d %d' % (obj.name, index, len(types_dict[obj.type_name])))
        print(' '.join(str(type_index[t]) for t in types_dict[obj.type_name]))

    #   5. Print canary and the number N of ground atoms in the initial
    # state. It is followed by a list of N lines, where each line has the name
    # of the atom, its index, the index of its predicate, a boolean number
    # indicating whether it is negated in the initial state, the number of
    # args, and the index of its objects.
    # It should be possible to identify the static predicates from this
    # information in the planner.
    print("INITIAL-STATE %d" % len(task.init))
    atom_index = {}
    for index, atom in enumerate(task.init):
        atom_index[str(atom)] = index
        # TODO what to do with functions?
        if isinstance(atom, pddl.Assign):
            continue
        print(atom, '%d %d %d %d' % (
            index, predicate_index[atom.predicate], atom.negated,
            len(atom.args)),
              ' '.join(str(object_index[o]) for o in atom.args))

    #   6. Print canary and the number of atoms in the goal, each atom is
    # followed by its index in the state.
    if isinstance(task.goal, pddl.Conjunction):
        goal_list = task.goal.parts
    else:
        goal_list = [task.goal]
    print("GOAL %d" % len(goal_list))
    for index, atom in enumerate(goal_list):
        print(atom, atom_index[str(atom)])

    #   7. Print the set of action schemas

    return


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
