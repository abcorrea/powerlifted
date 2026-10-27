#! /usr/bin/env python

import pddl


def conditional_effect_condition_literals(effect):
    condition = effect.condition
    if isinstance(condition, pddl.Literal):
        return [condition]
    if isinstance(condition, pddl.Truth) or condition is pddl.Truth:
        return []
    assert isinstance(condition, pddl.Conjunction), \
        "Effect condition is not normalized: %r" % condition
    return list(condition.parts)


def remove_unused_predicate_symbols(task):
    used_predicates = set()
    for a in task.actions:
        for p in a.get_action_preconditions:
            pred = p.predicate
            used_predicates.add(pred)
        for e in a.effects:
            l = e.literal
            pred = l.predicate
            used_predicates.add(pred)
            if e.is_conditional():
                for literal in conditional_effect_condition_literals(e):
                    used_predicates.add(literal.predicate)
    for p in task.init:
        pred = p.predicate
        used_predicates.add(pred)
    for axiom in task.axioms:
        used_predicates.add(axiom.name)
        condition = axiom.condition
        if isinstance(condition, pddl.Literal):
            body = [condition]
        else:
            body = condition.parts
        for literal in body:
            used_predicates.add(literal.predicate)
    goal_literals = []
    if isinstance(task.goal, pddl.Literal):
        goal_literals = [task.goal]
    elif isinstance(task.goal, pddl.Conjunction):
        goal_literals = list(task.goal.parts)
    for literal in goal_literals:
        used_predicates.add(literal.predicate)
    new_predicates = []
    for pred in task.predicates:
        if pred.name in used_predicates:
            new_predicates.append(pred)
    task.predicates = new_predicates
