# Development Notes

This directory contains the local development tests for Powerlifted. Its main
purpose is to support fast correctness checks while working on the planner.

## Contents

- `domains/`: small benchmark tasks used by the local test suite
- `run-tests.py`: local test runner
- `results.json`: sample timing baseline for comparison runs (machine dependent)

The local suite is intentionally small. It is meant to catch obvious
correctness regressions quickly before running larger experiments elsewhere.

## Local Test Coverage

`run-tests.py` currently covers four kinds of checks:

1. Core plan regression tests.
   These run `bfs` and `gbfs` with the `blind` heuristic on a small set of
   benchmark instances and check the expected plan cost.
2. Special-case planner paths.
   These include `clique_bk`, `clique_kckp`, a small object-creation task, and
   a disjunctive-precondition task that checks the Datalog heuristics on
   action schemas split by the translator (issue #64).
3. Heuristic smoke tests.
   These tests evaluate `goalcount`, `add`, `hmax`, `ff`, and `rff` through `gbfs`
   and require heuristic-evaluation output plus a valid plan.
4. Novelty / width-based smoke tests.
   These tests evaluate `bfws1`, `bfws1-rx`, `alt-bfws1`, and `dq-bfws1-rx`,
   including the novelty-related command-line flags.
5. Axiom (derived predicate) tests.
   The `axioms-*` domains cover recursion, multiple strata, existential body
   variables, constants and inequalities in axiom bodies, disjunctive and
   quantified goals, stratified negation (negated fluent and derived atoms
   in axiom bodies, negated derived preconditions), and derived predicates
   in goals and preconditions, across generators and heuristics (with
   pinned initial heuristic values). Exhaustive unsolvable instances check
   that derived atoms do not pollute duplicate detection, `axioms-invalid/`
   checks that the translator rejects tasks outside the supported fragment
   (including non-stratifiable axiom sets), and the stratification check
   has unit tests in `src/translator/tests/`. When `DOWNWARD_BENCHMARKS`
   is set, philosophers and optical-telegraphs are solved end to end with
   costs cross-checked against Fast Downward, and the psr domains are
   checked for clean rejection.
6. Negated-precondition and quantified-precondition tests.
   `negated-preconditions/` covers negated fluent/static preconditions and
   negated goals; `forall-preconditions/` covers universally and
   existentially quantified action preconditions (including plan printing
   of actions with non-external parameters); `inequality/` is a regression
   test for (in)equality filters on single-table join programs.

The local suite does not try to be exhaustive. The tests are also quite superficial,
and full experiments should be always used.

## Running The Local Suite

Build the planner first:

```bash
python build.py
```

Run the smaller developer suite:

```bash
python dev/run-tests.py --minimal
```

Run the full local suite:

```bash
python dev/run-tests.py
```

Store timings for later comparison:

```bash
python dev/run-tests.py --store-results dev/results.json
```

Compare the current run against a stored baseline:

```bash
python dev/run-tests.py --compare-results dev/results.json
```

## Important Caveats

- Run the test suite serially. The planner writes an intermediate
  translator file named `output.lifted` by default, so concurrent runs in the
  same working directory can clobber each other.
- Validation requires `validate` from [VAL](https://github.com/KCL-Planning/VAL)
  to be available on `PATH`.
- The suite is correctness-first, but some cases are still useful as rough
  performance sentinels. In particular, `join` on `organic-synthesis` is known
  to be much slower and more memory-hungry than the other local cases.

## When To Touch This Directory

Update `dev/` when a change affects:

- command-line behavior that should be covered by `CLI_OPTION_TESTS`
- search, heuristic, or successor-generator behavior that deserves a local
  regression case
- object-creation support
- local benchmarking expectations or comparison baselines

When adding a new local test, prefer a tiny instance with stable behavior and
clear assertions. The goal is to make failures easy to interpret.
