#! /usr/bin/env python

## Parses all effects and computes which predicates are static

def get_fluent_predicates_from_effects(actions):
    """Loops over all action effects and selects predicates that appear in some
    effect.  These predicates are fluent.  We return them to filter the ones not
    appearing in this set.

    """
    fluent_predicates = set()
    for a in actions:
        for e in a.effects:
            l = e.literal
            pred = l.predicate
            fluent_predicates.add(pred)
    return fluent_predicates

def get_str_predicates(predicates):
    """Creates a set of string containing the name of the predicates

    :param predicates: list of predicates of the type pddl.Predicate
    :return: set of strings corresponding to the name of each pdd.Predicate
    """
    new_predicates = set()
    for p in predicates:
        new_predicates.add(p.name)
    return new_predicates

def mark_static_predicates(task, static_predicates):
    """Mark predicates in the static_predicates set as static

    :param task: planning task in STRIPS format
    :param static_predicates: set of string containing all static predicates
    :return: void
    """
    for p in task.predicates:
        if p.name in static_predicates:
            p.set_static()
    return


def check(task):
    fluent_predicates = get_fluent_predicates_from_effects(task.actions)
    predicates_as_str = get_str_predicates(task.predicates)
    static_predicates = predicates_as_str - fluent_predicates
    mark_static_predicates(task, static_predicates)
    return static_predicates
