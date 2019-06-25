#! /usr/bin/env python

import itertools

import pddl


## Generates all possible ground atoms of a state

def get_ground_atoms(task, graph):
    """
    Loops over task and extracts all possible ground atoms (excluding static)

    :param task: Original task in STRIPS
    :param graph: Graph of types
    :return: set of pddl.Atom objects
    """
    ground_atoms = set()
    for pred in task.predicates:
        # Loop over all predicates and create lists of possible
        # objects for each argument
        instantiations = []
        for i in range(len(pred.arguments)):
            instantiations.append([])
        if pred.static == True:
            continue
        for index, arg in enumerate(pred.arguments):
            for obj in task.objects:
                # Gets all supertypes of the object.  If the argument is of
                # one of
                # these supertypes, then we can instantiate it with the object
                t = obj.type_name
                obj_supertypes = set()
                obj_supertypes.add(t)
                while t != 'object':
                    t = graph.edges[t]
                    obj_supertypes.add(t)
                if isinstance(arg.type_name, str):
                    if arg.type_name in obj_supertypes:
                        instantiations[index].append(obj.name)
                else:
                    # If it falls into this case, then it uses the 'either'
                    # type construction
                    if any(a in obj_supertypes for a in arg.type_name):
                        instantiations[index].append(obj.name)
        all_combinations = list(itertools.product(*instantiations))
        for comb in all_combinations:
            ground_atoms.add(pddl.Atom(pred.name, comb))
    return ground_atoms


def modify_initial_state(task, ground_atoms):
    """
    Check which fluent ground atoms are not in the initial state and add
    the negated atom for these.

    :param task: Original task in STRIPS
    :param ground_atoms: set of pddl.Atom objects
    :return: void
    """
    init_set = set(task.init)
    setview = set(ground_atoms)
    task.init += [atom.negate() for atom in setview - init_set]
    return


def generate_complete_initial_state(task, graph):
    """
    Change initial state of the task to have all ground atoms

    :param task: Original task in STRIPS
    :param graph: Graph of types
    :return: void
    """
    ground_atoms = get_ground_atoms(task, graph)
    modify_initial_state(task, ground_atoms)
    return
