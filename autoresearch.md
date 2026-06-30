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

## Baseline

Logged as run 1 in `autoresearch.jsonl`. Same code as the greedy-join baseline
(master HEAD + the meter): median **50.94 s** search_time (5 samples: 51.29,
50.82, 50.68, 51.55, 50.94; measured 2026-06-30), **47,644,479** grounder atoms /
**22,398,404** queue pushes (deterministic — identical on every sweep), peak
68 MB, 21-pair suite (7/7/7). Re-measure HEAD contemporaneously before any KEEP.

## What's Been Tried

**Theme so far: allocator churn per produced fact dominates, not the algorithm.**
Every kept win is "cheaper" (deterministic `grounder_atoms`/`grounder_pushes`
stay EXACTLY at baseline 47644479 / 22398404 — productions unchanged, behavior
preserved — while wall-clock drops). The deterministic counters are the
correctness net: any hashing/equality bug would move them. `perf` cannot sample
in this environment (0 samples even with `task-clock`), so wins are reasoned from
the alloc model and confirmed by contemporaneous A/B.

- **run 2 (KEEP, 5.9%) — defer achiever construction** (`weighted_grounder.cc`).
  Moved the cheapest-path check inline into project/join/product (dropped the
  batch `newfacts` buffer) and build the `Achievers` ONLY on the new-fact branch.
  Key insight: `reached_facts` achievers are dead (backchaining reads achievers
  from `lp`, written once at first insertion, never updated on a cheaper path),
  so the discard (53%) AND cheaper-path branches need no achiever at all. Also
  move-insert kept facts into `reached_facts`. commit cf27d27.
- **run 3 (KEEP, 20.2%) — inline `Arguments`**. Vendored `utils/small_vector.h`
  (SBO vector for trivially-copyable T, element-wise `==`, raw-pointer iterators
  ⇒ Fact hash/eq byte-identical) and backed `datalog::Arguments` with
  `small_vector<Term,4>`. Removes the per-produced-fact `std::vector<Term>` heap
  alloc (~47.6M). Had to qualify `std::find`/`std::distance` in join.h (raw-ptr
  iterators kill the prior std ADL). commit fe9b548.
- **run 4 (KEEP, 16.3%) — inline `Achievers`**. Backed `Achievers` with
  `small_vector<int,2>` + direct `int`/`int,int` ctors so project (1) and join
  (2) achiever bodies are built with no heap alloc (~22.4M new facts). commit
  924e16e. peak_mem 68→55MB.

- **run 5 (KEEP, 10.1%) — inline `JoinHashKey`** (`std::vector<int>` →
  `small_vector<int,3>`); per-join-firing key + stored map keys go off-heap.
  Local `JoinHashKeyHash` (same hash values as VectorHash). commit f8ede57.
- **run 6 (DISCARD) — in-place cheaper-path cost mutate** (idea #3). Counters
  flat, but A/B −1.1% (noise): the strictly-cheaper-path branch is too rare
  (most pushes are first-reaches) for removing its erase+insert to register.
- **run 7 (DISCARD) — union-SBO compact small_vector** (−8 bytes/vector, Fact
  ~76→60, peak_mem 55→48MB deterministically). But time only 0.32% (below noise),
  confidence FELL as reps grew. **KEY INSIGHT: reached_facts is local to each
  ground() call and cleared per state eval, so each set is small/cache-resident
  regardless of Fact size — the wins are from removing ALLOCATIONS (malloc/free
  cost), not object size. ⇒ size/N-tuning is a dead end; chase allocation +
  rehash COUNT.** Reverted (kept the simpler pointer-based small_vector).
- **run 8 (KEEP, 5.8%) — reserve `reached_facts`** to the previous call's size
  up front, skipping the ~log(n) doubling rehashes it paid growing from empty
  each call. Behaviour-neutral (set is only probed, never iterated). commit 3377ad4.
- **run 9 (KEEP, 4.5%) — reuse + pre-size join hash tables.** `clean_up` was
  `= JoinHashTable()` (frees both maps every call); now `reset_for_next_call()`
  clears the two OUTER maps in place and reserves them to the previous key count.
  Outer-map order is never used for productions, inner per-key sets stay fresh,
  so byte-identical. commit 221da57.

- **run 10 (DISCARD) — same-head-join collapse** (a "fewer" attempt): when a
  join's inverse condition contributes no head var, all partners give the same
  head, so emit one. Counters EXACTLY UNCHANGED ⇒ the branch never fired: the
  normal-form semijoin reduction (full_reducer/yannakakis) already projects away
  non-propagating vars, so the case doesn't exist here. Added check is overhead.
- **run 11 (DISCARD) — fuse find+insert into one `emplace` probe** (halves the
  ~22.4M new-branch double-probes). Byte-identical (counters flat, 0 trajectory
  moves) but A/B −0.3% (neutral): the saved probes are offset by ~25.2M extra
  inline-Fact moves emplace makes constructing+discarding a temp on every
  duplicate; the phmap SwissTable probe (compact control-byte scan) is too cheap
  for halving the probe count to win. ⇒ probe-COUNT reduction is also tapped out.

**Cumulative checkpoint (contemporaneous A/B, run 221da57 vs baseline f2ff317):
52.93s → 26.64s = 49.7% faster (~2× speedup), confidence 319×, grounder_atoms/
pushes IDENTICAL, peak_mem 68→55MB.** Every kept change behaviour-preserving.
**The lever was allocation/rehash COUNT in per-call hash containers (inline
small_vector + "reserve/reuse from the previous call").**

**Convergence assessment — the major levers are exhausted on this suite:**
- *Fewer productions* is empty: 88 rules after dedup (113 removed), EDB only 45
  atoms; the 47.6M productions / 53% waste are INHERENT (many body instantiations
  project to the same head). CSE/finalization/same-head-collapse all bottom out
  on work the normal-form transformation already did.
- *Cheaper per production*: allocations removed (small_vectors), per-call rehashing
  removed (reserve/reuse). Remaining per-production cost is the SwissTable probe +
  Fact copies into the join index — memory-latency-bound and already efficient
  (run 7: Fact size doesn't matter because SwissTable scans compact control bytes;
  run 11: halving probes doesn't help because probes are that cheap).
- Untried/rejected: join INNER per-key set reuse (changes partner order → changes
  recorded achiever → not byte-identical, risky); product-rule achiever vectors
  still std::vector (but product is rare — genome dominates with joins);
  incremental/delta grounding across consecutive states (big architectural change,
  out of scope). `perf` can't sample here (0 samples); callgrind too slow for a
  multi-eval search; gauge "fewer" ideas by whether grounder_atoms moves at all.

### THE run-11 "convergence" was WRONG — a second wave of STRUCTURAL wins (13-17)

After the allocation/reserve wins, removing that overhead UNMASKED slow data
structures and accidental work the allocator cost had hidden. Lesson: don't
declare convergence on "allocations are gone" — then hunt for slow std
containers, double lookups, accidental insertions, by-value returns, and
per-element work that can be precomputed.

- **run 13 (KEEP, 1.5%) — `find()` not `operator[]` for join partners.**
  `JoinHashTable::get_entries` used `hash_table[key]`, which INSERTS an empty
  entry on every partnerless-key lookup (common while a join fills; permanent for
  one-sided keys), polluting both maps. find()+static-empty is byte-identical.
  peak_mem 55→51MB. commit 359d50e.
- **run 14 (KEEP, 10.3%!) — flat_hash_map + single lookup for head positions.**
  `MapVariablePosition` was a `std::unordered_map<Term,int>` (node-based, pointer
  chase) AND `get_head_position_of_arg` did `count()`+`at()` (two lookups). It is
  consulted per body argument per produced head. Switched to phmap::flat_hash_map
  + one find(). A std::unordered_map on the hottest path was the single biggest
  hidden bottleneck. commit 4a42d75.
- **run 15 (KEEP, 5.1%) — precompute per-condition head positions.** The head
  position of each condition arg is fixed per rule; cache it (object/non-head →
  -1) so the join inner loops index an array instead of hashing each argument.
  commit 592b0bc.
- **run 16 (KEEP, 0.4%) — VariableSource::get_table() const returned BY VALUE**
  (a full vector copy) and backchain reads `get_table()[idx]` per variable per
  useful atom. Return const&. commit e7b30a7.
- **run 17 (KEEP, 0.6%) — reuse the run-15 precompute in the project rule.**
  commit ac968eb.
- **run 18 (DISCARD) — remove a redundant `Arguments(std::move())` temporary in
  DatalogAtom's ctor** (two inline moves → one, per Fact). Byte-identical but A/B
  within noise (0.23%, conf 0.60): an inline small_vector move is too cheap.

- **run 18 (DISCARD) — remove redundant `Arguments(std::move())` temporary** in
  DatalogAtom's ctor (two inline moves → one). Within noise (0.23%): inline move
  too cheap.
- **run 19 (KEEP, 1.9%) — cache rule type in the Match to kill virtual dispatch.**
  The grounder's per-popped-fact loop called the VIRTUAL `RuleBase::get_type()`
  up to twice per matched rule (the PROJECT/JOIN branch tests). Store the type in
  the `Match` struct (one get_type() per rule at matcher-build time) and dispatch
  on the int. A first cut that merely read get_type() ONCE was ~0.65% (RERUN);
  eliminating the virtual call entirely was ~3× better. commit 4765b0c.

**Cumulative (contemporaneous A/B, run 19 lineage vs baseline f2ff317): ~53.3s →
~22.2s ≈ 58% / 2.4×, grounder_atoms/pushes IDENTICAL, peak_mem 68→51MB.**

**Second-convergence assessment (more thorough than run 11's).** The in-scope
grounding hot path is now free of: per-fact heap allocs, per-call hash rehashing,
slow std containers, `operator[]` pollution, redundant per-element lookups,
by-value heavy returns, and virtual dispatch. Verified-marginal/blocked remainders:
- `AdaptiveQueue` push/pop dispatch (~45M virtual calls) is OUT OF SCOPE
  (priority_queues.h) and can't be safely replaced: forcing BucketQueue OOMs
  large-cost domains generally; forcing HeapQueue is slower + reorders pops
  (changes achievers). The adaptive fallback is load-bearing for correctness.
- `add_useful_atom` allocates a `GroundAtom` (==std::vector<int>, a core type in
  107 sites incl. states/novelty/successor-gen) per useful atom; useful_atoms'
  clear+rebuild contract is read by out-of-scope search code → can't inline/pool.
- `DatalogAtom::index = next_index++` per Fact is dead for facts but ~0.13% only.
- LightFact / index-only join inner set: the only remaining BIG copy saving, but
  both change partner ITERATION ORDER (int-hash vs args-hash, or grown capacity)
  ⇒ change the recorded first-achiever ⇒ not byte-identical (the relaxed gate
  MIGHT accept it if plan cost ≤ baseline, but it violates the achiever
  invariant; left for an explicit decision).
Lesson reinforced: after each apparent convergence, sweep for the NEXT class of
"wrong tool / accidental work" — two waves of wins came from doing exactly that.

## Measurement protocol (this box drifts ±10–15%)

Shared machine; whole-sweep timings drift on a minutes timescale. **Always A/B a
would-be KEEP contemporaneously**: build the candidate and a fresh build of
current HEAD back-to-back in one quiet window (`REPS=5` each), then
`decide.py --best <fresh-HEAD> --candidate <candidate>`. The deterministic
`grounder_atoms`/`grounder_pushes` counters do not drift — use them to *classify*
a change ("fewer" → counter drops; "cheaper" → counter flat) and to confirm a
"fewer" idea actually removed productions. Wait for 1-min load < ~1.6 and check
`pgrep -af perf` before timing.
