#! /usr/bin/env python3

"""Validation and stratification of axioms (derived predicates).

Powerlifted currently supports the following axiom fragment: after
normalization, every axiom body must be a conjunction of positive atoms,
possibly with (in)equality literals over '='. Variables occurring only in the
body are implicitly existentially quantified (they have been turned into
axiom parameters by the normalization). Positive recursion is allowed.

Everything else -- in particular any negated atom in an axiom body, and any
negated occurrence of a derived predicate in action preconditions or in the
goal -- is rejected here with an error message. The search component relies
on this module being the gatekeeper.

The stratifiability check implements the standard condition (see Thiebaux,
Hoffmann & Nebel, "In Defense of PDDL Axioms", AIJ 2005): build the
dependency graph over derived predicates, with an edge q -> p whenever q
occurs in the body of an axiom with head p, labeled negative if q occurs
negated. The axiom set is stratifiable iff no strongly connected component
contains a negative edge. Each derived predicate is assigned a stratum index
(a topological level of the SCC condensation); axioms are evaluated stratum
by stratum in the search component. With the positive-only fragment the
check trivially succeeds, but the general check is implemented (and unit
tested) so that support for negation can slot into it later.
"""

from collections import defaultdict

import pddl


class AxiomError(SystemExit):
    def __init__(self, message):
        SystemExit.__init__(self, "error: " + message)


def get_derived_predicates(task):
    """Return the set of names of all derived predicates of the task."""
    return {axiom.name for axiom in task.axioms}


def _describe_predicate(name, task):
    """Explain where an auxiliary derived predicate comes from, if it is one."""
    if not name.startswith("new-axiom@"):
        return "'%s'" % name
    if isinstance(task.goal, pddl.Atom) and task.goal.predicate == name:
        return ("'%s' (an auxiliary predicate introduced for the "
                "non-conjunctive goal)" % name)
    return ("'%s' (an auxiliary predicate introduced while normalizing a "
            "universally quantified or negated condition)" % name)


def _axiom_body_literals(axiom, task):
    """Return the list of body literals of a normalized axiom.

    Raises AxiomError if the body is not a conjunction of literals.
    """
    condition = axiom.condition
    if isinstance(condition, pddl.Truth):
        return []
    if isinstance(condition, pddl.Literal):
        return [condition]
    if isinstance(condition, pddl.Conjunction):
        for part in condition.parts:
            if not isinstance(part, pddl.Literal):
                raise AxiomError(
                    "the body of an axiom for %s contains an unsupported "
                    "construct (%s) after normalization"
                    % (_describe_predicate(axiom.name, task),
                       part.__class__.__name__))
        return list(condition.parts)
    raise AxiomError(
        "the body of an axiom for %s is not a conjunction after "
        "normalization (found %s)"
        % (_describe_predicate(axiom.name, task),
           condition.__class__.__name__))


def validate_axioms(task):
    """Check that all axioms fall into the supported fragment.

    Must be called after normalization. Axioms whose body simplified to
    falsity can never fire and are removed from the task.
    """
    derived = get_derived_predicates(task)

    kept_axioms = []
    for axiom in task.axioms:
        if isinstance(axiom.condition, pddl.Falsity):
            # The body is unsatisfiable; the axiom can never derive anything.
            continue
        for literal in _axiom_body_literals(axiom, task):
            if literal.negated and literal.predicate != "=":
                message = ("the body of an axiom for %s contains the negated "
                           "atom (not %s)"
                           % (_describe_predicate(axiom.name, task),
                              literal.negate()))
                if literal.predicate in derived:
                    message += ("; negation over the derived predicate %s is "
                                "not supported"
                                % _describe_predicate(literal.predicate, task))
                else:
                    message += ("; only positive atoms and (in)equalities are "
                                "supported in axiom bodies")
                raise AxiomError(message)
        kept_axioms.append(axiom)
    task.axioms = kept_axioms

    _reject_negated_derived_conditions(task, derived)


def _condition_literals(condition):
    """Return the literals of a normalized (conjunctive) condition.

    Unlike Action.get_action_preconditions, this also handles a condition
    that consists of a single negated literal.
    """
    if isinstance(condition, pddl.Literal):
        return [condition]
    if isinstance(condition, pddl.Conjunction):
        return [part for part in condition.parts
                if isinstance(part, pddl.Literal)]
    return []


def _reject_negated_derived_conditions(task, derived):
    """Derived predicates may only be used positively outside axiom bodies."""
    for action in task.actions:
        for literal in _condition_literals(action.precondition):
            if literal.negated and literal.predicate in derived:
                raise AxiomError(
                    "the precondition of action '%s' contains the negated "
                    "derived predicate %s; negation over derived predicates "
                    "is not supported"
                    % (action.name, _describe_predicate(literal.predicate,
                                                        task)))
    for literal in _condition_literals(task.goal):
        if literal.negated and literal.predicate in derived:
            raise AxiomError(
                "the goal contains the negated derived predicate %s; "
                "negation over derived predicates is not supported"
                % _describe_predicate(literal.predicate, task))


def check_stratification(derived_predicates, pos_edges, neg_edges):
    """Check stratifiability and compute a stratum index per derived predicate.

    :param derived_predicates: iterable of derived predicate names.
    :param pos_edges: set of pairs (q, p) such that q occurs positively in
        the body of an axiom with head p (only pairs of derived predicates).
    :param neg_edges: same for negated occurrences.
    :return: dict mapping each derived predicate to its stratum index. Strata
        are consecutive integers starting at 0, and whenever p depends on q,
        stratum(q) <= stratum(p), with strict inequality for negative edges.
    :raises AxiomError: if the axiom set is not stratifiable, i.e., some
        strongly connected component of the dependency graph contains a
        negative edge.
    """
    nodes = sorted(derived_predicates)
    successors = defaultdict(list)
    for (q, p) in sorted(pos_edges | neg_edges):
        successors[q].append(p)

    scc_of = _compute_sccs(nodes, successors)

    for (q, p) in sorted(neg_edges):
        if scc_of[q] == scc_of[p]:
            raise AxiomError(
                "the axioms are not stratifiable: derived predicate '%s' "
                "depends negatively on '%s' within a recursive cycle" % (p, q))

    # Assign a topological level to every SCC of the condensation: an SCC
    # with no dependencies gets level 0, every other SCC the maximum level
    # of its predecessors plus one. Predicates in the same SCC (mutual,
    # necessarily positive recursion) share a stratum.
    scc_predecessors = defaultdict(set)
    for (q, p) in sorted(pos_edges | neg_edges):
        if scc_of[q] != scc_of[p]:
            scc_predecessors[scc_of[p]].add(scc_of[q])

    level = {}

    def scc_level(scc):
        if scc in level:
            return level[scc]
        # The condensation is acyclic, so plain recursion terminates; use an
        # explicit stack to be robust for very deep chains.
        stack = [scc]
        while stack:
            current = stack[-1]
            pending = [p for p in scc_predecessors[current] if p not in level]
            if pending:
                stack.extend(pending)
                continue
            stack.pop()
            if current not in level:
                predecessor_levels = [level[p]
                                      for p in scc_predecessors[current]]
                level[current] = 1 + max(predecessor_levels, default=-1)
        return level[scc]

    return {pred: scc_level(scc_of[pred]) for pred in nodes}


def _compute_sccs(nodes, successors):
    """Iterative Tarjan; returns a dict mapping each node to its SCC id."""
    index_of = {}
    lowlink = {}
    on_stack = set()
    scc_of = {}
    tarjan_stack = []
    next_index = [0]
    next_scc = [0]

    for root in nodes:
        if root in index_of:
            continue
        work = [(root, iter(successors[root]))]
        index_of[root] = lowlink[root] = next_index[0]
        next_index[0] += 1
        tarjan_stack.append(root)
        on_stack.add(root)
        while work:
            node, it = work[-1]
            advanced = False
            for succ in it:
                if succ not in index_of:
                    index_of[succ] = lowlink[succ] = next_index[0]
                    next_index[0] += 1
                    tarjan_stack.append(succ)
                    on_stack.add(succ)
                    work.append((succ, iter(successors[succ])))
                    advanced = True
                    break
                elif succ in on_stack:
                    lowlink[node] = min(lowlink[node], index_of[succ])
            if advanced:
                continue
            work.pop()
            if work:
                parent = work[-1][0]
                lowlink[parent] = min(lowlink[parent], lowlink[node])
            if lowlink[node] == index_of[node]:
                while True:
                    member = tarjan_stack.pop()
                    on_stack.remove(member)
                    scc_of[member] = next_scc[0]
                    if member == node:
                        break
                next_scc[0] += 1
    return scc_of


def compute_axiom_strata(task):
    """Compute the stratum of every derived predicate of the (valid) task.

    :return: pair (strata, num_strata), where strata maps each derived
        predicate name to its stratum index.
    """
    derived = get_derived_predicates(task)
    pos_edges = set()
    neg_edges = set()
    for axiom in task.axioms:
        for literal in _axiom_body_literals(axiom, task):
            if literal.predicate not in derived:
                continue
            if literal.negated:
                neg_edges.add((literal.predicate, axiom.name))
            else:
                pos_edges.add((literal.predicate, axiom.name))
    strata = check_stratification(derived, pos_edges, neg_edges)
    num_strata = 1 + max(strata.values(), default=-1)
    return strata, num_strata
