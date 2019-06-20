#! /usr/bin/env python

import copy
import sys

import pddl

from collections import defaultdict

## Preprocess object types of the task.
## The following operations are performed:
##
##   - Compile types into unary predicates;
##   - Add unary type predicates as action preconditions;
##   - Add unary type and supertype predicates to initial state;
##   - Remove types from action parameters (see comment in __str__ of TypedObject);

name_to_type_pred = {}

class TypesGraph(object):
    """Create a graph representing the hierarchy of types and supertypes.  Every
    type points to its supertype.
    """
    def __init__(self, types):
        self.types = types
        self.edges = self.create_graph(self.types)

    def create_graph(self, types):
        g = defaultdict(str)
        for t in types:
            g[t.name] = t.basetype_name
        return g

def compile_into_unary_predicates(task):
    """Create one new unary predicate for each object type."""
    for t in task.types:
        pred_name = _get_type_predicate_name(str(t))
        task.predicates.append(pddl.Predicate(pred_name, ['?x']))
        name_to_type_pred[pred_name] = task.predicates[-1]
    return

def add_conditions_to_actions(task, graph):
    """Add unary type conditions to each action schema according to their
    parameters.

    """
    for action in task.actions:
        for param in action.parameters:
            name = param.name
            param_type = param.type_name
            types = []
            action.precondition.add_condition(pddl.Atom(_get_type_predicate_name(param_type), [name]))

def adjust_initial_state(task, graph):
    """Creates the unary predicates for types and supertypes of each object and
    append them to the initial state description.

    """
    for obj in task.objects:
        type_name = obj.type_name
        name = obj.name
        types = []
        while type_name != 'object':
            # While there is a supertype, append this to the list of predicates
            # being added in the initial state.
            types.append(type_name)
            type_name = graph.edges[type_name]
        for t in types:
            task.init.append(pddl.Atom(_get_type_predicate_name(t), [name]))

def _get_type_predicate_name(t):
    return 'type_' + t

def compile_types(task):
    graph = TypesGraph(task.types)
    compile_into_unary_predicates(task)
    add_conditions_to_actions(task, graph)
    adjust_initial_state(task, graph)
    return graph
