#! /usr/bin/env python

import itertools

import pddl

from collections import defaultdict


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

    :param graph:
    :return:
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


def compute_initial_state(task, reachable_atoms, static):
    map_pred_instantiations = defaultdict(list)
    for p in task.predicates:
        for i in p.arguments:
            map_pred_instantiations[p.name].append(set())
    for atom in task.init:
        for index, arg in enumerate(atom.args):
            for p, i in reachable_atoms[(atom.predicate, index)]:
                map_pred_instantiations[p][i].add(arg)

    ground_atoms = set()
    for pred, instantiations in map_pred_instantiations.items():
        combinations = list(itertools.product(*instantiations))
        for comb in combinations:
            ground_atoms.add(pddl.Atom(pred, comb))

    task.init = list(set(task.init).union(ground_atoms))

    return


def generate_overapproximated_reachable_atoms(task):
    graph, static = create_flow_graph(task)
    reachable_atoms = compute_reachability(graph)
    compute_initial_state(task, reachable_atoms, static)
    return
