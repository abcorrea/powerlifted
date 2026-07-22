#! /usr/bin/env python3

"""Unit tests for the axiom stratification check (axiom_rules.py).

Run directly: python3 src/translator/tests/test_axiom_rules.py
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                os.pardir))

import axiom_rules


def expect_strata(derived, pos_edges, neg_edges, expected):
    strata = axiom_rules.check_stratification(derived, pos_edges, neg_edges)
    assert strata == expected, "expected %r, got %r" % (expected, strata)


def expect_not_stratifiable(derived, pos_edges, neg_edges):
    try:
        axiom_rules.check_stratification(derived, pos_edges, neg_edges)
    except SystemExit as e:
        assert "not stratifiable" in str(e), str(e)
        return
    assert False, "expected stratification to fail for %r %r %r" % (
        derived, pos_edges, neg_edges)


def test_empty():
    expect_strata(set(), set(), set(), {})


def test_single_recursive_predicate():
    # reachable depends positively on itself: one stratum.
    expect_strata({"reachable"}, {("reachable", "reachable")}, set(),
                  {"reachable": 0})


def test_chain_of_strata():
    # p2 depends on p1 depends on p0: three strata.
    expect_strata({"p0", "p1", "p2"},
                  {("p0", "p1"), ("p1", "p2")}, set(),
                  {"p0": 0, "p1": 1, "p2": 2})


def test_mutual_recursion_shares_stratum():
    # p and q derive each other (positively) and r builds on top of them.
    expect_strata({"p", "q", "r"},
                  {("p", "q"), ("q", "p"), ("q", "r")}, set(),
                  {"p": 0, "q": 0, "r": 1})


def test_independent_predicates_may_share_level():
    expect_strata({"p", "q"}, set(), set(), {"p": 0, "q": 0})


def test_negation_across_strata_is_fine():
    # q :- not p is stratifiable: p in a lower stratum than q.
    expect_strata({"p", "q"}, set(), {("p", "q")}, {"p": 0, "q": 1})


def test_negative_self_loop_rejected():
    # p :- not p.
    expect_not_stratifiable({"p"}, set(), {("p", "p")})


def test_negative_edge_in_cycle_rejected():
    # p :- q and q :- not p: the SCC {p, q} contains a negative edge.
    expect_not_stratifiable({"p", "q"}, {("q", "p")}, {("p", "q")})


def test_larger_cycle_with_negation_rejected():
    # p -> q -> r -> p positively, plus r depends negatively on q.
    expect_not_stratifiable(
        {"p", "q", "r"},
        {("p", "q"), ("q", "r"), ("r", "p")},
        {("q", "r")})


def test_negation_into_recursive_component_is_fine():
    # r and s are mutually recursive; both depend negatively on p, which is
    # in its own (lower) stratum.
    expect_strata({"p", "r", "s"},
                  {("r", "s"), ("s", "r")},
                  {("p", "r"), ("p", "s")},
                  {"p": 0, "r": 1, "s": 1})


def main():
    failures = 0
    tests = [(name, fn) for name, fn in sorted(globals().items())
             if name.startswith("test_") and callable(fn)]
    for name, fn in tests:
        try:
            fn()
            print("PASSED %s" % name)
        except AssertionError as e:
            failures += 1
            print("FAILED %s: %s" % (name, e))
    if failures:
        sys.exit("%d stratification unit test(s) failed" % failures)
    print("All %d stratification unit tests passed." % len(tests))


if __name__ == "__main__":
    main()
