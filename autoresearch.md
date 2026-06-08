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
./autoresearch.checks.sh   # dependency guard + full regression suite
```

First runs `autoresearch.guard.sh` (rejects any external `#include`, e.g.
boost — see Constraints), then the full local regression suite
(`dev/run-tests.py`). Takes ~40 s; 62 tests. Run it serially, never in
parallel with a sweep. Any failure (guard or tests) reverts the experiment.

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

- `autoresearch.sh`, `autoresearch.checks.sh`, `autoresearch.guard.sh`,
  `benchmarks/**` (suite, PDDLs, `reference.json`, `run_suite.py`) — the agent
  never edits the judge, the exam, or the dependency guard.
- `dev/**` — the regression suite and its expected plan costs.
- `.claude/**`, `autoresearch.jsonl` history (append-only), `driver/**`
  (not measured), `build.py`/CMake flags (changing optimization flags is
  not an interesting research result; `-O3` etc. stay as they are).

## Constraints

- Behavior-preserving only (see contract above).
- C++17; keep the code style of the surrounding files.
- **No new dependencies.** Only the C++ standard library and the deps already
  vendored in-tree (`src/search/parallel_hashmap/`) are allowed. **No boost, no
  other third-party/system library.** boost headers happen to live in
  `/usr/include`, so `#include <boost/...>` compiles with no CMake/link change
  and the *build* will not catch it — but it is NOT a project dependency.
  `autoresearch.checks.sh` runs `autoresearch.guard.sh`, which fails on any
  external include; treat that failure like any other check failure (revert).
  If a win genuinely needs a container the stdlib lacks (e.g. a small-buffer-
  optimized vector), **vendor a minimal single-header implementation into the
  repo** (the way phmap is vendored) — never `#include <boost/...>`.
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

**Current best: run 12, commit `0531a7a`** (median ≈ 42 s, measured clean;
baseline was 81.2 s → ~48 % cumulative). peak_mem rose to **184 MB** (run 12
tradeoff, see below). Stored medians drift with the machine — when you need the
real bar, re-measure HEAD contemporaneously (see protocol below).

**BREAKTHROUGH (runs 10 + 12): memory LAYOUT beats memory ALLOCATION.** Run 4's
"malloc-shaving is dead — tcache makes small allocs free" was only half the
story: it tested alloc *count*, not *layout*. SBO (small-buffer-optimized
vectors, inline storage) wins on cache locality + pointer-indirection on hot
hash/compare/copy paths — a different axis. Two big wins from one lens:
- **run 10 (+21 %)**: grounder `Arguments`/`Achievers` → small_vector.
- **run 12 (+12 %)**: successor-gen `GroundAtom`/`Table::tuple_t` → small_vector.
**The live direction: find hot tiny heap-vectors on a dominant path, inline
them.** Both kept; together they cut the suite ~31 % off the run-6 baseline.

**Where to go next (prioritized, for a resuming agent).**
1. **Recover run-12's memory regression (tune SBO N / decouple types).** Run 12
   took peak_mem 149→184 MB (+23 %) by making GroundAtom AND Table::tuple_t a
   shared `small_vector<int,4>`. Try: (a) smaller inline N (2 or 3 — relation
   arities are small) and re-measure time+mem; (b) DECOUPLE — keep GroundAtom
   small (persistent, in relations + packer maps) and tuple_t wider (transient,
   join width), converting element-wise in select_tuples (same cost as the
   existing copy). Goal: hold the ~12 % speed while clawing back memory. Cheap to
   screen (rebuild + REPS=3 shows both metrics). **DO THIS FIRST** — it's a
   strict-improvement refinement of a just-kept win.
2. **Re-profile post-run-12** to find the new bottleneck. Pre-run-12 grounder
   showed a now-hot `JoinHashTable` (`flat_hash_map<vector<int>,JoinHashEntry,
   VectorHash<int>>` try_emplace ~6 %) keyed by `JoinHashKey = std::vector<int>`
   — an SBO candidate (small key copied per fact insert), contained to
   rules/join.h + VectorHash. Also `Arguments::Arguments` copy ~3.7 %.
3. **Join-path bundle** (`autoresearch-data/pending-join-path-bundle.patch`):
   phmap + index-store + in-place semi-join compaction. Real but INTRINSICALLY
   sub-floor on this box (~5-6 %, confidence 0.66-1.73x across runs 8/9/11) — do
   NOT retry alone. Only land it bundled with another successor-gen win that
   pushes the combined effect clearly past the gate, or for its −10 MB mem win
   if memory becomes the priority.
4. **State packing / grounder algorithmic** — see older notes below; higher risk.

NOTE: runs 10+12 originally used a header-only **system-boost** include
(`boost/container/small_vector.hpp`). **boost is NOT a project dependency and is
now prohibited** — see Constraints → "No new dependencies", enforced by
`autoresearch.guard.sh` (run from `autoresearch.checks.sh`). The small_vector
wins are being preserved by **vendoring a minimal single-header `small_vector`
into the tree** (the way phmap is vendored), not by depending on boost. Do NOT
reintroduce `#include <boost/...>` (or any external include) — the checks gate
rejects it and the experiment must be reverted.

Reminder: only wins ≥ ~5 % are confirmable here — BATCH small ones or find a
single ≥6 % structural change. (Runs 10 + 12 were each well over.)

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

Profile note: the **Datalog grounder** (`src/search/datalog/`) dominates the
FF/hmax/add configs (rerun once per state), while the **database-join
successor generator** (`src/search/successor_generators/`, `database/`)
dominates the blind configs and is on every config's expansion path. By
generator, **full_reducer ≈ 61 % of suite time**, yannakakis ≈ 31 %, join
≈ 9 %; by evaluator, ff ≈ 47 %, blind ≈ 38 %, hmax ≈ 16 %. Both sides have
yielded wins (grounder: run 2; successor gen: run 6) — do NOT assume "all
leverage is in the grounder" (an earlier note said that; run 6 disproved it).
See `autoresearch.ideas.md` for the live backlog.

### Wins
- **run 12 (KEEP, ~12 %)** — SBO on the successor-gen path: `GroundAtom` and
  `Table::tuple_t` (`std::vector<int>` → shared `boost small_vector<int,4>`), so
  every relation tuple and join intermediate (hash_join/semi_join/project/
  cartesian/filter_static) is inline. Targets the blind/full_reducer hot path
  (hash_semi_join 18 % + ~21 % malloc) that run 10 didn't touch. Behavior-
  preserving (content-based TupleHash/feed keep set iteration order → identical
  packer indices → identical trajectory; 62/62 + gate). Pooled 10v10 A/B:
  41.79 vs 47.68 = +12.35 %, confidence 2.06x. **Tradeoff: peak_mem 149→184 MB
  (+23 %)** — see next-steps #1 to recover it. Cascade touched ~15 files
  (templated TupleHash, added small_vector feed, AtomicGoal/added_atoms/
  pack_tuple/order_tuple → GroundAtom; LiftedOperatorId kept vector<int>).
- **run 10 (KEEP, ~21 %)** — SBO for the grounder's per-fact heap vectors:
  `Arguments` → `boost::container::small_vector<Term,4>` and `Achievers` →
  `small_vector<int,2>`. Inlines the two tiny heap buffers every Fact carried,
  removing both the per-fact allocs and the pointer indirection on every fact
  hash/compare/copy in the per-state Datalog fixpoint (grounder = 63 % of
  suite). Behavior-preserving storage-only change (62/62 + reference gate);
  needed one ADL fix in join.h (`std::find`/`std::distance`). Confirmed +21.15 %
  (confidence 18.2x) in a REPS=5 contemporaneous A/B (45.96 vs 58.29). peak_mem
  flat at 149 MB. **Lesson: memory LAYOUT (SBO/locality) is a live lever even
  where malloc-shaving (alloc count) was dead — different axis (see run 4).**
- **run 2 (KEEP, ~15–17 %)** — three behavior-preserving grounder overhead
  cuts, committed together: (a) `MapVariablePosition` flat vector + single
  scan replacing `unordered_map<Term,int>` and its double lookup; (b)
  `JoinRule::clean_up` uses `JoinHashTable::clear()` (retain buckets) instead
  of reassigning a fresh table; (c) `WeightedGrounder::join` builds its key in
  a reused member buffer instead of allocating per call.
- **run 6 (KEEP, ~9.8 %)** — cut redundant successor-generator work (the
  full_reducer path = ~61 % of suite). The decisive piece: **`filter_static`
  no longer re-checks an already-enforced static (in)equality precondition
  after every join** — once its columns are in the working table, later joins
  preserve them, so re-filtering is a no-op; track applied preconds with a
  per-`instantiate` bitset. Enabled for the linear-join generators
  (generic/full_reducer/ordered); Yannakakis keeps original behavior (it
  projects columns away per subtree, breaking the invariant). Bundled with the
  run-5 patch: sort-hoist in `pack` + `apply_lifted_action_effects`
  O(n) `std::find`→`insert().second`. The sort-hoist/find were sub-floor
  alone; **`filter_static` dedup was what pushed the bundle to 9.8 %.**
  Takeaway: **algorithmic redundancy on the dominant path beats micro-opts.**

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
- **run 5 (DISCARD, but REAL ~3.7 % — patch saved)** — successor-generator /
  state-packer wins: (1) hoist `std::sort` out of `SparseStatePacker::pack`'s
  relation loop (it re-sorted the whole growing vector after every relation;
  R sorts → 1, identical final sorted vector) + drop a dead `packed_relation`
  alloc; (2) `apply_lifted_action_effects` replace an O(n) `std::find` over an
  `unordered_set` with `insert().second` (O(1)). Behavior-preserving (62/62),
  simpler code, and a **consistent ~3.7 %** in a pooled 10v10 contemporaneous
  A/B (62.83 vs 65.23) — but the win is **below this machine's ~5 % noise
  floor**, so decide.py's min-rel gate can never confirm it (no number of reps
  helps: the gate needs win > baseline CV ≈ 5 %). **These two changes were
  later bundled with the filter_static dedup and committed as run 6** — they
  ride along in the kept commit, just couldn't carry it alone.
- **run 7 (DISCARD)** — precompute `get_indices_and_constants` per fluent atom
  into `PrecompiledActionData` (the code's own TODO) + `pack()` reserve total
  tuples instead of `num_predicates`. Behavior-preserving (62/62) but a
  contemporaneous A/B showed no gain (candidate 61.7 vs run-6 HEAD 59.2). Once
  the algorithmic redundancies are gone, trimming tiny state-independent
  recompute + fixing reserves does not move the metric. Don't retry this class.
- **run 8 (DISCARD — strongest near-miss; PATCH SAVED)** — join-path bundle:
  (a) `hash_join`'s `std::unordered_map` and `hash_semi_join`'s
  `std::unordered_set` → **phmap** (SwissTable, the project's standard map),
  behavior-preserving since join output order depends on t1/t2 iteration, not
  map layout; (b) `hash_join`'s build map stores **indices into `t1.tuples`**
  instead of copying whole tuples (valid — `t1.tuples` is untouched until the
  final move). Behavior-preserving (62/62 + gate). Over a **15v15** pooled
  contemporaneous A/B (3 rounds): bundle median 58.4 vs HEAD 61.1 = consistent
  **~4.5 % faster (mean 3.7 %)** AND a **deterministic peak_mem 139 vs 149 MB
  (-7 %)** — but confidence only 0.66x because ~4.5 % sits below this machine's
  effective noise floor (~6.6 %, inflated by persistent slow-sweep outliers).
  Real, clean, lower-memory — but unconfirmable on the primary metric. Patch:
  `autoresearch-data/pending-phmap-join-wins.patch`
  (`git apply` to re-add). **TODO: bundle with a 3rd join-path win** (e.g.
  precompute `compute_matching_columns` for the semi-join order — its tables
  have fixed `tuple_index`; or store indices in `hash_semi_join` too) to push
  the combined win past ~6 % so it clears. phmap (a) alone was ~2.9 % (also
  sub-floor); the index-store (b) added the memory win + a bit more speed.

### Profile map (where the time is)
- **FF/hmax/add/bfws configs** → dominated by the **Datalog grounder**
  (`src/search/datalog/`), esp. `WeightedGrounder::join`. Heavily optimized in
  run 2; further malloc/hash shaving there is dead (run 4).
- **blind configs** (and the successor side of every config) → the **database
  join successor generator** (`src/search/successor_generators/`,
  `database/`): `std::sort` in state packing (run 5), `GenericJoinSuccessor::
  filter_static` (does linear `find`s over `tuple_index` — TODO in code says
  preprocess them), `hash_join`/`hash_semi_join`, and the full relation copy
  in `generate_successor` (`vector<Relation> new_relation(state.get_relations())`
  copies every relation's `unordered_set` each successor — biggest single
  structural cost, but copy-on-write is invasive/risky).

### The confirmable-floor problem (decisive constraint)
This machine's run-to-run CV is ~5 %. decide.py only returns KEEP when the win
is BOTH ≥2× the noise AND larger than baseline CV. So **only wins ≥ ~5 % are
confirmable here, period.** Sub-5 % real wins (runs 4 nominal, 5) cannot pass
no matter how many reps. Strategy that follows: **don't chase 3–4 % wins
individually — batch enough behavior-preserving cuts into ONE experiment to
clear ~6 %, or find a single ≥6 % structural change.** Run 2 (~15 %) is the
proof this works.

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
