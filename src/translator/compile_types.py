#! /usr/bin/env python

from collections import defaultdict

import pddl

## Preprocess object types of the task.
## The following operations are performed:
##
##   - Compile types into unary predicates;
##   - Add unary type predicates as action preconditions;
##   - Add unary type and supertype predicates to initial state;
##   - Remove types from action parameters (see comment in __str__ of
# TypedObject);

name_to_type_pred = {}


class TypesGraph(object):
    """
    Create a graph representing the hierarchy of types and supertypes.  Every
    type points to its supertype.
    """

    def __init__(self, types):
        # self.type is list of types and self.edges is a dictionary where
        # each type points to its supertype.
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
        obj_in_action = set()
        for param in action.parameters:
            name = param.name
            param_type = param.type_name
            obj_in_action.add((param_type, name))
        # Loop over preconditions and effects, collect all parameters
        # and constants, and add types to that in the precondition
        precond = action.get_action_preconditions
        for cond in precond:
            assert isinstance(cond, pddl.Literal)
            for arg in cond.args:
                name = arg
                param_type = next(
                    (x.type_name for x in action.parameters if x.name == arg),
                    None)
                if param_type is None:
                    # If the type is none, then it is not a parameter and
                    # it must be constant.  THus, we search for its type in the
                    # obj list.
                    param_type = next(
                        (x.type_name for x in task.objects if x.name == arg),
                        None)
                obj_in_action.add((param_type, name))
        literals_in_effects = action.get_literals_in_effects
        for l in literals_in_effects:
            name = l
            param_type = next(
                (x.type_name for x in action.parameters if x.name == l), None)
            if param_type is None:
                # If the type is none, then it is not a parameter and
                # it must be constant.  THus, we search for its type in the
                # obj list.
                param_type = next(
                    (x.type_name for x in task.objects if x.name == l), None)
            obj_in_action.add((param_type, name))

        action.transform_precondition_into_list()
        for obj in obj_in_action:
            param_type = obj[0]
            name = obj[1]
            action.precondition.add_condition(
                pddl.Atom(_get_type_predicate_name(param_type), [name]))
    return


def adjust_initial_state(task, graph):
    """Creates the unary predicates for types and supertypes of each object and
    append them to the initial state description.

    """
    for obj in task.objects:
        type_name = obj.type_name
        name = obj.name
        types = set()
        types.add(type_name)
        while type_name != 'object':
            # While there is a supertype, append this to the list of predicates
            # being added in the initial state.
            types.add(type_name)
            type_name = graph.edges[type_name]
        for t in types:
            task.init.append(pddl.Atom(_get_type_predicate_name(t), [name]))


def _get_type_predicate_name(t):
    return 'type@' + t


def remove_trivially_inapplicable_actions(task, graph):
    """
    Remove actions that are never instantiated.  This occurs once a parameter
    has a given type T but there is no object with such parameter.

    :param task:
    :param graph:
    :return:
    """
    object_types_in_task = set()
    for obj in task.objects:
        type_name = obj.type_name
        object_types_in_task.add(type_name)
        while type_name != 'object':
            # While there is a supertype, append this to the list of types
            # appearing in any object of the task
            object_types_in_task.add(type_name)
            type_name = graph.edges[type_name]

    new_actions = set()
    for action in task.actions:
        keep_action = True
        for param in action.parameters:
            if param.type_name not in object_types_in_task:
                keep_action = False
        if keep_action:
            new_actions.add(action)
        else:
            print ("Removing action %s" % action.name)

    task.actions = new_actions

def compile_types(task):
    """
    Compile types into static unary predicates, modify the precondition of
    actions and the initial state accordingly.

    It also creates a graph where each node is a type and it has an outgoing
    edge to its unique supertype.

    :param task: STRIPS task
    :return: void
    """
    graph = TypesGraph(task.types)
    compile_into_unary_predicates(task)
    add_conditions_to_actions(task, graph)
    adjust_initial_state(task, graph)
    remove_trivially_inapplicable_actions(task, graph)
    return graph
