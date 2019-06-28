#! /usr/bin/env python

import itertools
from collections import defaultdict

import pddl


# Generates initial grounded states using overapproximation of the
# reachability analysis.
#
# We first perform a reachability analysis of the parameters of the
# predicates, then we use such reachability to overapproximate the set of
# ground atoms in the initial state.

def create_flow_graph(task):
    """
    Create a graph where each node corresponds to a pair of predicate and
    parameter, represented by its index. A node (P, i) has an outgoing edge
    to node (Q, j) if an object X appears in the ith position of P in the
    precondition of some action A and it also appears as the jth argument of
    Q in the effects of A.

    :param task: STRIPS task
    :return: graph where each node is a parit (P, i) where P is an n-ary
    predicate and i <= n is a parameter index
    """
    # First, filter static predicates and initialize graph
    static = set()
    graph = defaultdict()
    for pred in task.predicates:
        if pred.static:
            static.add(pred.name)
        for index, _ in enumerate(pred.arguments):
            graph[(pred.name, index)] = []

    for action in task.actions:
        map_pred_arg_index = defaultdict(int)
        map_obj_pred = defaultdict(list)
        for prec in action.precondition.parts:
            for index, arg in enumerate(prec.args):
                map_pred_arg_index[(prec.predicate, arg)] = index
                map_obj_pred[arg].append(prec.predicate)
        for eff in action.effects:
            l = eff.literal
            for index, arg in enumerate(l.args):
                for pred in map_obj_pred[arg]:
                    reverse_index = map_pred_arg_index[(pred, arg)]
                    graph[(pred, reverse_index)].append((l.predicate, index))

    return graph, static


def compute_reachability(graph):
    """
    Compute reachability of pairs (P, i) where P is a predicate and i is the
    parameter ith parameter of P.  Use a BFS to compute it.

    :param graph: graph structure where each node is a (P, i)
    :return: dictionary K -> [A,B,...] where K is a pair (P, i),
    with predicate P and positional argument i, and [A,B,...] are lists of
    pairs (Q,j) reachable from K
    """
    reachable_atoms = defaultdict(list)
    # Compute reachability using BFS for each node
    for node in graph:
        queue = graph[node].copy()
        while len(queue):
            v = queue.pop()
            if v in reachable_atoms[node]:
                continue
            reachable_atoms[node].append(v)
            for child in reachable_atoms[v]:
                queue.append(child)

    return reachable_atoms


def compute_dict_obj_types(task, g):
    """
    Compute a dictionary {O: t} where O is an object and t is a list of its
    types.

    :param task: STRIPS task
    :param g: type graph
    :return: dictionary object -> list of types
    """
    object_types = defaultdict(list)
    for obj in task.objects:
        t = obj.type_name
        object_types[obj.name].append(t)
        while t != "object":
            t = g.edges[t]
            object_types[obj.name].append(t)

    return object_types


def compute_dict_arg_types(task):
    """
    Creates a dictionary (P: t) where P is a predicate and t is a list of its
    argument types.

    :param task: STRIPS Task
    :return: dictionary of predicate -> list of argument types
    """
    arg_types = defaultdict(list)
    for pred in task.predicates:
        for arg in pred.arguments:
            if not isinstance(arg, str):
                arg_types[pred.name].append(arg.type_name)

    return arg_types


def compute_initial_state(task, reachable_atoms, type_graph, static):
    """
    Modify the initial state of the task.  Start by computing two
    dictionaries, one to keep track of the object types and supertypes and
    the other to keep track of the types of the argument of each predicate.

    Then, we create a dictionary of list of lists.  Each key of this
    dictionary is a predicate of the task and it has one list for each of its
    parameter.  Thus, we have a list for each argument of each predicate.

    We loop over all atoms in the initial state, check which arguments they
    might reach (based on the reachable_atoms structure) and insert them to
    this argument list in the corresponding predicate.

    Last, we generate all combinations of arguments for each predicates and
    update the initial state accordingly.

    :param task: STRIPS task
    :param reachable_atoms: dictionary K -> [A,B,...] where K is a pair (P,
    i), with predicate P and positional argument i, and [A,B,...] are lists
    of pairs (Q,j) reachable from K
    :param type_graph: TypeGraph of the task
    :param static: set of strings representing the static predicates
    :return: void
    """
    # Compute dictionary {O : t*} where O is an object and t* is a list of
    # types.
    object_types = compute_dict_obj_types(task, type_graph)
    # Compute dictionary {P : t1,...,tn} where O is a predicate and t1,...,
    # tn is a list of types for its n arguments
    arg_types = compute_dict_arg_types(task)

    map_pred_instantiations = defaultdict(list)
    for p in task.predicates:
        for i in p.arguments:
            map_pred_instantiations[p.name].append(set())
    for atom in task.init:
        if isinstance(atom, pddl.Assign):
            # If it is a numeric constant, we skip it
            continue
        for index, arg in enumerate(atom.args):
            for p, i in reachable_atoms[(atom.predicate, index)]:
                if p not in static:
                    assert not isinstance(arg_types[p][i], list)
                    if arg_types[p][i] in object_types[arg]:
                        map_pred_instantiations[p][i].add(arg)
                    if arg == 'roomb':
                        continue

    # for action in task.actions:
    #     args_in_precond = set()
    #     if isinstance(action.precondition, pddl.Literal):
    #         precond = [action.precondition]
    #     else:
    #         precond = action.precondition.parts
    #     for p in precond:
    #         for arg in p.args:
    #             args_in_precond.add(arg)
    #     for e in action.effects:
    #         for index, arg in enumerate(e.literal.args):
    #             if arg not in args_in_precond:
    #                 continue


    ground_atoms = set()
    for pred, instantiations in map_pred_instantiations.items():
        combinations = list(itertools.product(*instantiations))
        for comb in combinations:
            ground_atoms.add(pddl.Atom(pred, comb))

    init_set = set(task.init)
    task.init += [atom.negate() for atom in ground_atoms - init_set]

    return


def generate_overapproximated_reachable_atoms(task, type_graph):
    """
    Computes an overapproximation of the reachability, where an object O is
    considered reachable to argument j of a predicate Q if O occurs as the
    ith argument of a predicate P and there is an action schema such that P
    appears in the precondition with ith argument '?x' and Q is in the effect
    with '?x' as its jth argument.

    This computation is done in several steps.  This reachability analysis is
    done using a BFS in lifted representation and then we loop over the
    initial state to instantiate all possible combinations given this analysis.

    This function also modifies the initial state accordingly to the
    reachability of the objects.

    :param task: STRIPS task
    :param type_graph:  TypeGraph of the task
    :return: void
    """
    graph, static = create_flow_graph(task)
    reachable_atoms = compute_reachability(graph)
    compute_initial_state(task, reachable_atoms, type_graph, static)
    return
