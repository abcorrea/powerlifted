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

**Current best: run 2, commit `23d9f30`.** Its *stored* samples have a
median of 67.0 s, but that run landed in a slow machine window — measured
clean it runs ≈ **63 s**. Do NOT trust the stored 67.0 as the bar.

**Measurement protocol (learned the hard way — follow it):** this box drifts
±10–15 % on a minutes timescale, so comparing a candidate against *stored*
best samples produces both false negatives (a real win buried under a slow
candidate window) and false positives (a neutral change looking great against
a slow stored baseline — run 4 nearly did this). **Always A/B
contemporaneously:** measure the candidate AND a fresh build of current HEAD
back-to-back in the same quiet window (REPS=5 each), then
`decide.py --best <fresh-HEAD> --candidate <candidate>`. Only commit on KEEP
of that fresh comparison. Yes, it doubles the cost (~16 min) — it is the only
honest signal on this machine. (Stash the candidate, `build.py`, measure HEAD,
unstash, `build.py`, measure candidate.)

Profile (perf, 4 representative pairs): the **Datalog grounder**
(`src/search/datalog/`) dominates *every* config — it is the FF/hmax/add
heuristic + reachability engine, rerun once per state. Top symbols:
`WeightedGrounder::join` (8–14 %), allocation (malloc/free/new ~16–20 %
combined), the `get_head_position_of_arg` lookup, phmap `FlatHashSet<Fact>`
inserts, `JoinRule::clean_up`. Almost all leverage is here, not in the
successor generators. See `autoresearch.ideas.md`.

### Wins
- **run 2 (KEEP, ~15–17 %)** — three behavior-preserving grounder overhead
  cuts, committed together: (a) `MapVariablePosition` flat vector + single
  scan replacing `unordered_map<Term,int>` and its double lookup; (b)
  `JoinRule::clean_up` uses `JoinHashTable::clear()` (retain buckets) instead
  of reassigning a fresh table; (c) `WeightedGrounder::join` builds its key in
  a reused member buffer instead of allocating per call.

### Insights (read before measuring!)
- **NEVER run anything CPU-heavy concurrently with a sweep, including
  `perf record`.** `perf` pinned to core 2 (the sweep core) silently poisoned
  a whole REPS=5 run (samples jumped to 90–115 s). Profile only when no sweep
  is timing. Check `pgrep -af perf` and `/proc/loadavg` (1-min) before timing.
- **Machine noise floor ≈ 5–9 % CV**, and it drifts on a minutes timescale
  (whole-sweep multiplicative; a single REPS=5 run can ramp 76→94 s within
  it). Consequence: a lone ~5 % micro-opt will NOT earn a KEEP — confidence
  stalls at ~1.4–1.8x. **Batch several related, individually-justified
  behavior-preserving micro-opts into one experiment** so the combined effect
  (~10–15 %) clears the gate. This is how run 2 passed (each of its 3 parts
  was sub-floor alone; the flat-vector part alone only ever hit 1.0–1.8x).
- Wait for 1-min load < ~1.6 before timing (shared box). Loads settle in a
  few minutes after stopping background work.
- The recorded **run-1 baseline is noisy** (a contaminated 96.1 s sample
  inflates its MAD); clean baselines measure ~78 s. For honest A/B on this
  drifting box, measure candidate and a contemporaneous control close together.

### Dead ends
- **run 3 (DISCARD)** — `reached_facts`/`newfacts` as reused members (retain
  buckets across ground()) + hoist of the join inner-loop head-position
  lookup. No gain over run 2 and +6 MB peak. Once the run-2 wins are in, the
  per-state set/vector allocations and that lookup are no longer dominant.
- **run 4 (DISCARD)** — Achievers by-value sink ctor + `std::move` at the
  join/product sites (kills a double `vector<int>` alloc per fact) +
  `is_cheapest_path` `lazy_emplace` with in-place cost mutation (one Fact hash
  instead of find+erase+insert). All behavior-preserving (62/62). Looked like
  ~6 % vs the stored run-2 best, but a **contemporaneous A/B** showed it dead
  even with run 2 (both ≈ 63.2 s, confidence 0.02x). **Grounder micro-
  allocation/hashing cuts do not help past run 2** — glibc tcache makes these
  tiny frequent allocations near-free; the cost is in the probing/copying
  work, not the malloc calls. Next: algorithmic/structural changes, or the
  successor generators (state side), not more malloc-shaving.

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
