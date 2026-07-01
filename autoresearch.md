# Autoresearch: Powerlifted grounder-work optimization

A fresh agent should be able to read this file and run the loop. If you are
that agent: read this whole file, the tail of `autoresearch.jsonl`, and
`git log --oneline -20`, then continue experimenting. Never stop to ask
permission; the user interrupts when they want to steer.

## Objective

Make the Datalog grounder do **less work per state** so the add/ff heuristics
evaluate faster — **without degrading the plans found**. The grounder is re-run
once per state by the add/ff heuristics, so cutting its per-call work pays off on
every heuristic evaluation. This loop is the re-scope after the greedy-join loop
proved the join *order* is already well-tuned (see "Why this target" below); the
lever now is **redundant work inside the grounder**, not join ordering.

The **primary metric** is the summed search-component time (`Total time`) over
the suite (`benchmarks/suite.json`). Lower is better.

A **secondary, deterministic signal** is the cumulative grounder work
(`grounder_atoms` = facts produced, `grounder_pushes` = priority-queue pushes),
reported by an in-binary meter. It is **noise-free** (same code + same trajectory
→ identical counts).

**CRITICAL — how to read the screen on THIS loop (differs from the join loop):**
grounder efficiency has two sub-levers, and they show up differently:

- **Produce *fewer* facts** (algorithmic: skip doomed derivations, share
  sub-joins) → `grounder_atoms` drops **and** time drops. The deterministic
  counter screens these for free.
- **Produce the *same* facts more cheaply** (less allocation / hashing per fact)
  → `grounder_atoms` is **flat** but time still drops.

So **do NOT auto-discard an idea just because `grounder_atoms` is flat** — that
was the right rule for join ordering, but here a flat-counter change can be a
real wall-clock win. Use `grounder_atoms` to *classify* a win (fewer vs cheaper),
not to gate it. Time is the gate.

The configurations under test (search/evaluator/generator):

- `gbfs / add / full_reducer`   — h^add; grounder value is order/work-independent.
- `gbfs / ff / full_reducer`    — h^ff; relaxed plan can shift on a behavior change.
- `alt-bfws1 / ff / yannakakis` — h^ff, the default satisficing config.

## The headline: ~53% of grounder productions are wasted

At baseline the grounder produces **47.64 M** facts but only **22.40 M** of them
are ever queued (`grounder_pushes`); the other **~25.2 M (53%)** are produced and
immediately discarded as `HAS_CHEAPER_PATH` (the head atom was already reached at
≤ cost). This holds across every config (add 52%, ff 56%, alt-bfws1 48%). That
wasted half is the leverage this loop chases.

The grounder is a Dijkstra-like best-first **weighted fixpoint**
(`WeightedGrounder::ground` in `src/search/datalog/grounder/weighted_grounder.cc`):
a priority queue ordered by cost; each popped fact fires its matching rules
(`project`/`join`/`product`), each produced head goes through
`is_cheapest_path_to_achieve_fact`, which keeps it only if new or strictly
cheaper. `Fact` hashes/compares on **predicate + arguments only** (not cost, not
achievers — see `datalog_fact.h`), which is what makes several of the ideas below
safe and cheap.

## The target levers

1. **(c) Cut the cost of the wasted 53%.** The hottest path is
   `WeightedGrounder::join`: for every already-reached partner of the join key it
   builds `new_arguments` (copy) **and** an `achievers_body` vector **and** a full
   `Fact` (with `Achievers`) — then `is_cheapest_path_to_achieve_fact` throws
   ~half of them away. Because `Fact` equality is predicate+args only, you can
   **probe `reached_facts` with the head atom *before* constructing the
   `Achievers`**, and only build the achiever/Fact when it will be kept. That
   removes ~25 M achiever allocations (time win; `grounder_atoms` stays flat —
   this is a "cheaper", not "fewer", win, so judge it on time).
2. **(c) Skip producing doomed facts entirely (fewer).** In a non-negative-weight
   Dijkstra fixpoint a fact is *final* once popped. If you track finalized facts,
   join/project can skip emitting heads already finalized — fewer productions
   (`grounder_atoms` drops). **Verify the pop order is non-decreasing for h^add
   AND h^max** before relying on finalization (it should be; confirm, don't
   assume), and keep it behavior-preserving (a skipped doomed fact must not change
   any kept fact's cost or recorded achiever).
3. **`is_cheapest_path` churn.** On the strictly-cheaper branch it does
   `erase` then `insert` (two hashes) and a separate `find`. A single in-place
   update (e.g. `lazy_emplace`/iterator mutation) would hash once. Small, broad.
4. **(a) Partial common-subexpression sharing across rules.** Full-equivalence
   CSE already runs (`remove_duplicate_rules`, iterated; `collapse_predicates` is
   on by default) — but only merges *wholly identical* rules. Many action rules
   share a *sub*-join (a 2-atom precondition pattern) without being identical;
   factoring shared sub-joins into one aux predicate computed once cuts repeated
   derivation (`grounder_atoms` drops). Higher effort; `remove_duplicate_rules`
   even has a "TODO Do in a smarter way" hook.
5. **`product()` / cartesian path.** The product rule builds a deque-driven
   cartesian product copying `Arguments` per partial; check for redundant copies.

Profile first (`perf record` a single heavy pair when no sweep is timing) to see
which of join/project/product/is_cheapest dominates before picking.

## How to run

```bash
./autoresearch.sh          # rebuild + sanity + warm-up + 3 timed sweeps
```

Prints per sweep: `METRIC search_time=<s>` (primary), `METRIC grounder_atoms=<n>`
/ `grounder_pushes=<n>` (deterministic screen), `METRIC peak_mem_mb=<MB>`.
Non-zero exit = crash (build/sanity failure, timeout/OOM, unsolved pair, or a
plan-cost regression). Per-pair reports land in
`autoresearch-data/sweep-<rep>.json`.

- Per-task limits: 300 s wall-clock, 8 GiB memory. Strictly serial; nothing else
  CPU-heavy (incl. `perf`) while a sweep is timed. `REPS=n`, `CORE=n` env vars.

Correctness gate, after a passing benchmark, off the metric clock:

```bash
./autoresearch.checks.sh   # dependency guard (no external includes) + dev/run-tests.py
```

## Decision procedure

```bash
python3 .claude/skills/autoresearch/scripts/decide.py \
    --best "<best kept search_time samples>" \
    --candidate "<this run's search_time samples>" \
    --direction lower
```

`KEEP` → commit; `DISCARD`/crash/checks-fail → revert (`git checkout -- .` +
remove new files by hand; **never `git clean`**); `RERUN`/`NEED_MORE_REPS` → pool
more samples. Log every experiment as one line in `autoresearch.jsonl` (include
the `grounder_atoms` delta and whether the win is "fewer" or "cheaper"). Because
the available win is large (~53% waste), real wins should clear the ~5% time
noise floor — but confirm with a contemporaneous A/B (see Measurement protocol).

## Behavior contract (RELAXED gate)

`benchmarks/reference.json` stores per pair `solved`, `cost` (gated) plus
`expansions`, `generations`, `search_time`, `grounder_atoms`, `grounder_pushes`
(diagnostics). Gate:

- **Every pair the baseline solved must still solve**, with **plan cost ≤
  baseline**. Cost regression exits 3; unsolved exits 2.
- Trajectory (expansions/generations) is **not** gated; the harness reports moves
  as `NOTE trajectory moved: ...`.

Why relaxed: most grounder-work cuts are exactly behavior-preserving for **h^add**
(its value is the order/work-independent least fixpoint — expansions must NOT
move; a trajectory note on an add pair means you changed a value, i.e. broke
something). For **h^ff** the relaxed plan can shift via achiever tie-breaking, so
trajectory may move there as long as the plan still solves at ≤ cost. **Any
optimization that drops or reorders productions must not change a kept fact's
final cost or its recorded best achiever** — that is the invariant that keeps
both heuristics correct. `./autoresearch.checks.sh` (62 tests with expected plan
costs) is the backstop. **Never regenerate `benchmarks/reference.json`.**

## Files in scope

- `src/search/datalog/grounder/**` (the weighted grounder — primary lever) and
  `src/search/datalog/rules/**`, `src/search/datalog/transformations/**` (for CSE).
- Rest of `src/search/datalog/**` — fair game if a change needs support.
- `src/translator/**` — allowed but its time is NOT in the metric.

## Off-limits

- `autoresearch.sh`, `autoresearch.checks.sh`, `autoresearch.guard.sh`,
  `benchmarks/**` (suite, PDDLs, `reference.json`, `run_suite.py`).
- **The meter**: `src/search/datalog/grounder/grounder_statistics.h` and its call
  sites (`record_grounder_run` in `weighted_grounder.cc`, `print_grounder_statistics`
  in `main.cc`). Counting must stay honest — do not move/remove these. If you
  change *how many* facts are produced, the counter SHOULD move with it (that is
  the point); just never edit the counting itself.
- `dev/**`, `.claude/**`, `autoresearch.jsonl` history (append-only),
  `build.py`/CMake flags.

## Constraints

- Stay within the relaxed gate (solved + cost ≤ baseline). Preserve every kept
  fact's final cost and recorded achiever.
- **No new dependencies.** Stdlib + in-tree vendored deps only
  (`parallel_hashmap/`, `utils/small_vector.h`) — **no boost, no other
  third-party/system library**; the guard enforces it.
- C++17; match the surrounding style. Simpler is better. One experiment at a
  time; finish (commit or revert) before the next.

## Why this target (carried over from the greedy-join loop)

The prior loop (branch `autoresearch/greedy-join-2026-06-29`, 11 experiments, all
discarded) established that FD's greedy join *order* is robust and near-optimal
among static strategies on this suite — every cost-model / cardinality / tiebreak
variant either tied, bloated grounder work, or OOM'd, because this grounder is a
weighted fixpoint (not a one-shot DB join) and the dominant-cost domain
(genome-edit-distance, ~59% of grounder work) has small bodies with no ordering
leverage. The leverage that *does* exist is redundant *work* (the 53% wasted
productions and repeated sub-joins), which is what this loop targets.

**Base note:** this branch is cut from master `6a46eb8`, which already includes
the merged search-time optimizations (the run-2 grounder lookup/allocation cuts
and the small_vector SBO on `GroundAtom`/`Table` tuples). Those are DONE — do not
redo them. Importantly the grounder still produces **47.64 M atoms with 53%
waste** on top of them (SBO changed memory layout, not fact counts), so the
redundant-work lever is untouched by what's already merged.

## Baseline

Logged as run 1 in `autoresearch.jsonl`. Master `6a46eb8` + the meter, 21-pair
suite (7/7/7), measured 2026-07-01 in a quiet window (loadavg ~1.1).

- **search_time:** median **27.97 s** (5 samples: 29.82, 29.84, 27.97, 27.86,
  27.89). NOTE this is **~28 s, not the ~51 s** an earlier *stale-base*
  measurement showed: current master carries the merged search-time optimizations
  (successor-gen + `GroundAtom`/`Table` SBO), which nearly halve wall-clock on
  this suite. Re-measure HEAD contemporaneously before any KEEP (box drifts).
- **Deterministic anchor (load-independent):** **47,644,479** grounder atoms /
  **22,398,404** queue pushes — identical on every sweep; **~53% wasted
  productions** (25.2 M). peak 65 MB. Those opts left grounder fact counts
  unchanged, so the redundant-work lever is intact.

## What's Been Tried

Experiments run 2–6 (run 1 = baseline). Only run 2 kept.

- **run 2 — KEEP (+4.5%), "cheaper", counter flat.** Build join/project achiever
  bodies directly in the inline `small_vector<int,2>` via new
  `Achievers(int,int,int,int)` / `Achievers(int,int,int)` ctors, removing the
  per-produced-fact heap-allocated temporary `std::vector<int>` (join 2-int +
  project 1-int). Committed `3c51e36`. **The winning pattern: delete a genuine
  per-fact heap allocation.**
- **run 3 — DISCARD, "cheaper".** `ReachedFacts::get_costs()` by-value →
  const-ref (stop copying the costs vector in product's min scan). Real but
  tiny; only touches the narrow product path; +0.4% is lost in noise.
- **run 4 — DISCARD, "cheaper".** Rewrite `is_cheapest_path` with phmap
  `lazy_emplace` to fuse the `find()`+`insert()` double-probe into one, plus
  in-place `const_cast` cost update on the strictly-cheaper branch (both
  behavior-preserving — reached_facts achievers are never read back; backchain
  reads achievers from `Datalog::facts`). Plan costs identical. But the saved
  find-probe on small cache-hot genome args is too cheap: order-balanced A/B
  gave ~0.7% median with ±1–2s noise → indistinguishable from zero.
- **run 5 — DISCARD, REGRESSED −1.2%.** Make `reached_facts`/`newfacts` reused
  members `clear()`-ed per call. `clear()` is O(capacity); a bucket array sized
  to the biggest eval penalizes the many small evals. Reuse is *worse* here.
- **run 6 — DISCARD, REGRESSED −0.7%.** Hoist join's partner (head_pos,
  arg_pos) map out of the per-partner loop. `position_of` is already a cache-hot
  ≤4-elem scan; the precompute pass then runs unconditionally, adding cost to
  the many zero-partner join calls.

**Takeaways.** (1) The grounder is already well-tuned post-SBO+run2; hot funcs
(join 15%, product 11%, phmap prepare_insert 9.6%, is_cheapest 7.8%) are
dominated by *irreducible* work (Arguments copies, Fact construction, hash
probes on real data), not removable overhead. (2) Removing a real per-fact heap
alloc wins (run 2); shaving cache-hot scalar work or fusing cheap probes does
not; reusing big containers or adding unconditional precompute *regresses*.
(3) Remaining clear target = the ~11% still in malloc/free, mostly product's
`std::vector<int>`/`deque` (genome). (4) **Next levers to try:** product
cartesian allocation (lever 5) and the count-reducing levers (finalization
lever 2 / CSE lever 4) that drop `grounder_atoms` rather than per-fact cost.

**Measurement protocol that works on this box (IMPORTANT):** naive 5-vs-5
fresh-build A/B is too noisy (±3–5%, box drifts within a sweep). Instead: build
BOTH binaries once, save them, then run **order-balanced interleaved** full-suite
sweeps (alternate which binary runs first each round) via `run_suite.py` with the
saved binary copied over `builds/release/search/search`. In a quiet window this
gives a **0.3–0.5% noise floor** and detects sub-1% effects. Always verify
`grounder_atoms` flat (cheaper win) and plan cost preserved on a heavy pair
(genome d-13-2 gbfs-ff = cost 79, d-7-8 gbfs-add = cost 31) before timing.

## Measurement protocol (this box drifts ±10–15%)

Shared machine; whole-sweep timings drift on a minutes timescale. **Always A/B a
would-be KEEP contemporaneously**: build the candidate and a fresh build of
current HEAD back-to-back in one quiet window (`REPS=5` each), then
`decide.py --best <fresh-HEAD> --candidate <candidate>`. The deterministic
`grounder_atoms`/`grounder_pushes` counters do not drift — use them to *classify*
a change ("fewer" → counter drops; "cheaper" → counter flat) and to confirm a
"fewer" idea actually removed productions. Wait for 1-min load < ~1.6 and check
`pgrep -af perf` before timing.
