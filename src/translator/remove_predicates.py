#! /usr/bin/env python


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
    for p in task.init:
        pred = p.predicate
        used_predicates.add(pred)
    new_predicates = []
    for pred in task.predicates:
        if pred.name in used_predicates:
            new_predicates.append(pred)
    task.predicates = new_predicates
