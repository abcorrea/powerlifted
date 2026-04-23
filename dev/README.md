# Development Notes

This directory contains the local development assets for Powerlifted. Its main
purpose is to support fast regression checks while working on the planner.

## Contents

- `domains/`: small benchmark tasks used by the local regression suite
- `run-tests.py`: local regression runner
- `results.json`: sample timing baseline for comparison runs

The local suite is intentionally small. It is meant to catch obvious
correctness regressions quickly before running larger experiments elsewhere.

## Local Test Coverage

`run-tests.py` currently covers four kinds of checks:

1. Core plan regression tests.
   These run `bfs` and `gbfs` with the `blind` heuristic on a small set of
   benchmark instances and check the expected plan cost.
2. Special-case planner paths.
   These include `clique_bk`, `clique_kckp`, and a small object-creation task.
3. Heuristic smoke tests.
   These exercise `goalcount`, `add`, `hmax`, `ff`, and `rff` through `gbfs`
   and require heuristic-evaluation output plus a validated plan.
4. Novelty / width-based smoke tests.
   These exercise `bfws1`, `bfws1-rx`, `alt-bfws1`, and `dq-bfws1-rx`,
   including the novelty-related command-line flags used in practice.

The local suite does not try to be exhaustive. It favors short runs with real
end-to-end coverage over large benchmark sweeps.

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

If you prefer `uv`, the same commands can be run with `uv run`.

## Important Caveats

- Run the regression suite serially. The planner writes an intermediate
  translator file named `output.lifted` by default, so concurrent runs in the
  same working directory can clobber each other.
- Validation requires `validate` from VAL to be available on `PATH`.
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
