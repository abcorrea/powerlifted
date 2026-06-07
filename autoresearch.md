# Autoresearch: Powerlifted search-time optimization

A fresh agent should be able to read this file and run the loop. If you are
that agent: read this whole file, the tail of `autoresearch.jsonl`, and
`git log --oneline -20`, then continue experimenting. Never stop to ask
permission; the user interrupts when they want to steer.

## Objective

Make the Powerlifted search component faster **without changing its
behavior**. The primary metric is the summed search-component time
(`Total time` as reported by the search binary itself) over a fixed suite of
34 (config, instance) pairs drawn from the HTG benchmark set. Lower is
better.

The six planner configurations under test (search/evaluator/generator):

- `alt-bfws1 / ff / yannakakis` (default satisficing config)
- `bfs / blind / full_reducer`
- `bfs / blind / join`
- `bfws1-rx / blind / full_reducer`
- `gbfs / ff / full_reducer`
- `gbfs / hmax / full_reducer`

The suite is defined in `benchmarks/suite.json`; PDDL files live in
`benchmarks/htg/<domain>/`. Each config is paired with instances it solves
in roughly 1.5–5 s, one per HTG domain family. All tasks are translated
with `--unit-cost`, matching the reference experiment the suite was drawn
from (`2026-04-24-A-powerlifted-thesis-simul`).

## How to run

```bash
./autoresearch.sh          # rebuild + sanity + warm-up + 3 timed sweeps
```

Prints one `METRIC search_time=<seconds>` line per timed sweep (plus a
secondary `METRIC peak_mem_mb=<MB>`). Each sweep takes ~80 s; a full
harness invocation (build + sanity + warm-up + 3 reps) takes ~6 min.
Non-zero exit = crash (build failure, sanity failure, per-task timeout/OOM,
or behavior mismatch — see below). Per-pair timings for the last invocation
land in `autoresearch-data/sweep-<rep>.json` — use them to see *where* a
delta comes from.

**Noise profile of this machine** (shared box, `powersave` governor):
occasional sweeps run uniformly ~10–20 % slower or faster — whole-sweep,
multiplicative events, not pair-specific. The decision script's median+MAD
statistics absorb them (baseline MAD ≈ 1.6 s ≈ 2 % despite a 73–96 s sample
range), but the consequence is: screen ideas at `REPS=3`, and **confirm any
would-be KEEP with a fresh `REPS=5` run before committing.** Expect wins to
need ≥ ~5 % to clear the gates; that is the machine's honest detection
floor.

- Per-task limits: 300 s wall-clock, 8 GiB memory (translator and search).
- Strictly serial: never run two planner executions in parallel (they are
  pinned to one core, and concurrency would poison the timings anyway).
- Do not run anything else CPU-heavy while a sweep is being timed.
- `REPS=n ./autoresearch.sh` for more repetitions; `CORE=n` to change the
  pinned core (default 2).

Correctness gate, run after a *passing* benchmark, off the metric clock:

```bash
./autoresearch.checks.sh   # full local regression suite (dev/run-tests.py)
```

Takes ~40 s; 62 tests. Run it serially, never in parallel with a sweep.

## Decision procedure

```bash
python3 .claude/skills/autoresearch/scripts/decide.py \
    --best "<best kept samples>" --candidate "<this run's samples>" \
    --direction lower
```

Act on the `verdict` field: `KEEP` → commit; `DISCARD`/crash/checks-fail →
revert (`git checkout -- .` + remove new files by hand; never `git clean`);
`RERUN` → pool more samples (confirm at most ~2×, then discard);
`NEED_MORE_REPS` → run again. Log every experiment as one line in
`autoresearch.jsonl` (samples included).

## Behavior-preservation contract

`benchmarks/reference.json` stores, for every pair: exit code, solved flag,
plan cost, expansions, and generations. Every sweep is compared against it
and the harness exits 3 on any mismatch. This is the contract that keeps
experiments honest: only pure performance changes (data structures, memory
layout, hashing, successor-generation internals, avoiding redundant work,
...) are allowed — anything that alters the search trajectory is out, even
if it looks like an improvement.

**Never regenerate `benchmarks/reference.json` to make a failing experiment
pass.** It changes only if the user explicitly approves a behavior change.

## Files in scope

- `src/search/**` — the C++ search component. This is where the metric
  lives; almost all experiments belong here.
- `src/translator/**` — allowed, but translation time is *not* part of the
  metric. Only worth touching if it speeds up *search* (e.g., a better
  intermediate representation) while keeping behavior identical. The
  translator cache (`benchmarks/.cache/`) auto-invalidates on any change to
  the built translator sources.

## Off-limits

- `autoresearch.sh`, `autoresearch.checks.sh`, `benchmarks/**` (suite,
  PDDLs, `reference.json`, `run_suite.py`) — the agent never edits the
  judge or the exam.
- `dev/**` — the regression suite and its expected plan costs.
- `.claude/**`, `autoresearch.jsonl` history (append-only), `driver/**`
  (not measured), `build.py`/CMake flags (changing optimization flags is
  not an interesting research result; `-O3` etc. stay as they are).

## Constraints

- Behavior-preserving only (see contract above).
- C++17; keep the code style of the surrounding files.
- Simpler is better: removing code while holding the metric is a win; ugly
  complexity for a tiny gain is a discard.
- One experiment at a time; finish (commit or revert) before starting the
  next.

## Baseline

Logged as run 1 in `autoresearch.jsonl` (segment 0): median **81.2 s**
(5 samples: 81.2, 82.8, 96.1, 81.2, 73.5), peak memory 149 MB, measured
2026-06-07 on this machine. The config header in the ledger defines the
metric; re-init with a new header if the machine or the suite ever changes.

## What's Been Tried

(Nothing yet — loop not started. Append wins, dead ends, and insights here
every 5–10 experiments. Check this list and `git log` before retrying
anything similar.)

## Idea backlog

See `autoresearch.ideas.md` if present; create it when ideas outpace
experiments. Starting points worth profiling first:

- `perf record` a sweep subset to find the hot loops per generator
  (`yannakakis` vs `full_reducer` vs `join` stress different code).
- Hash-join / semi-join internals in `src/search/database/`.
- State packing / hashing (`src/search/states/`, `hash_structures.cc`).
- Successor-generator table copies and allocations.
- Novelty-evaluation data structures (`src/search/novelty/`) for the BFWS
  configs.
- FF/hmax heuristic recomputation (`src/search/heuristics/`,
  `delete_relaxation_heuristics/`) — memoization that provably cannot
  change returned values.
