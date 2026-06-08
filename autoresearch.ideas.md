# Idea backlog

Profile (perf, 4 representative pairs, 2026-06-07): the **Datalog grounder**
(`src/search/datalog/`) dominates every config — it's the FF/hmax/bfws
heuristic + reachability engine, rerun per state. Hot symbols across pairs:

- `WeightedGrounder::join`            8–14 %
- `malloc` / `cfree` / `operator new` ~16–20 % combined (allocation churn)
- `std::_Hashtable<Term,int>::find`   4.6–6 %  (= `get_head_position_of_arg`,
  which does a DOUBLE unordered_map lookup per call)
- `phmap ... FlatHashSet<Fact>::prepare_insert` 4–7 % (reached-facts set +
  join hash entries)
- `WeightedGrounder::ground` / `product` / `is_cheapest_path_to_achieve_fact`
- `JoinRule::clean_up` ~2–3 % (rebuilds two phmap tables every ground() call)

## Live ideas (cheap → expensive)

DONE in run 2 (committed 23d9f30): old ideas 1, 3, 4 (flat-vector lookup,
reused join key buffer, clean_up clear()).

2. Hoist `get_head_position_of_arg` out of join's inner loop over matched
   facts — the (cond-pos → head-pos) mapping is loop-invariant. Precompute
   once per join() call (into a reused buffer to avoid per-call alloc). Note:
   with the flat-vector lookup already in, the per-element scan is cheap, so
   this is now a smaller win — bundle it with others.
5. [DONE/DEAD in run 4] `is_cheapest_path` lazy_emplace single-hash — no gain.
6. [DEAD in run 3] reuse reached_facts member — no gain.
7. [DONE in run 10, +21 % KEEP — THE BIG WIN] Fact stores Arguments
   (vector<Term>) + Achievers (vector<int>). Gave both SBO (small_vector<Term,4>
   / small_vector<int,2>). Run 4's "malloc isn't the bottleneck" was about alloc
   COUNT; SBO wins on LAYOUT/cache-locality + indirection (every fact
   hash/compare/copy in the fixpoint), a different axis. **Now apply the same SBO
   lens elsewhere — see "SBO targets" below.**
8. [DONE/DEAD vs behavior] `JoinHashEntry` as vector instead of set — the set
   dedup is load-bearing (cost-lowering re-inserts), and storing fact indices
   would use live (updated) costs vs the set's snapshot cost → behavior change.
   Skip.
9. [partly DONE in run 6; recompute-elim DEAD in run 7] successor-gen
   redundancy. STILL OPEN:
   - `filter_static` Yannakakis path still re-checks every call (left original
     for safety). A correct per-subtree dedup could help the yannakakis configs
     (alt-bfws1-ff-yannakakis is the heaviest single config), but reason
     carefully about projection (columns get projected away → single bitset is
     wrong; needs per-subtree tracking). NOTE run 7 suggests recompute-elim is
     low value, so weight this accordingly.
   - DEAD (run 7): precompute get_indices_and_constants into adata + pack
     reserve total tuples → no measurable gain. The per-state recompute and the
     reserve were not bottlenecks. Don't retry recompute/reserve micro-opts.
10. `generate_successor` copies ALL relations of the state
    (`vector<Relation> new_relation(state.get_relations())`, each an
    unordered_set) for every successor — likely the biggest single structural
    cost on the successor side. Copy-on-write / structural sharing would cut it
    but is invasive and risks DBState equality/hashing behavior. High
    risk/high reward; do as a dedicated careful experiment.
11. `JoinHashKey` single-int specialized path for single-var joins. Structural,
    modest.
12. Algorithmic: hmax/add grounder reruns the whole Datalog fixpoint per state
    from scratch. **MEASURED (instrumented ground(), 4 hmax domains): caching the
    STATIC closure is worthless — the static-only fixpoint (permanent EDB, no
    state facts) derives +0 in genome & organic-MIT (closure == raw EDB) and only
    ~20%/<1% of the full closure in pipesworld/visitall. Relaxed-action rules all
    need a fluent precondition, so ~80-99% of each per-state closure is genuinely
    state-driven and cannot be precomputed. The permanent EDB is tiny (18-196
    facts, ~2-10% of work) so re-seeding it is already cheap. DO NOT chase
    "cache the static closure".** The only surviving angle is PARENT->CHILD reuse
    (successive states differ by a few effect atoms) — but it's much harder and
    risky: visitall shows 2 state facts -> 27575 reached, i.e. the relaxed closure
    can swing violently on a tiny state delta, so keeping h-values bit-identical
    across incremental-vs-fresh is very hard. Not low-hanging.

## Join-path bundle (runs 8 + 9 — DEAD as a standalone win; patch saved)
`autoresearch-data/pending-join-path-bundle.patch` (supersedes the old
`pending-phmap-join-wins.patch`) holds the **full 3-part join bundle**:
phmap for hash_join/hash_semi_join + index-storage in hash_join's build map
(run 8) + **in-place stable compaction in hash_semi_join** (run 9: move
survivors down instead of copying into a fresh `new_tuples`; drops one vector
alloc + N tuple copies per semi-join).

**Verdict after run 9: the join path does NOT contain a 6 % win.** Adding the
3rd win (in-place semi-join) on top of run-8's ~4.5 % moved the metric only
~0.25 % (4.75 % total in a REPS=5 A/B; deterministic peak_mem 139 vs 149 MB,
-6.7 %). Still sub-floor (4.75 % < the machine's 5-9 % CV). The semi-join copies
are NOT a bottleneck — glibc tcache makes the allocs near-free (same lesson as
run 4). **Do not keep adding micro-wins to this bundle.** The only way it lands
is to **bundle it with a separately-confirmed STRUCTURAL win** (e.g. the
generate_successor relation-copy, idea 10) so the COMBINED effect clears ~6 %.
Plan: confirm the structural win alone first, commit it, then re-apply this
patch on top and A/B the combination.

Tried-and-not-worth-pursuing on this path: precompute `compute_matching_columns`
(state-independent, but it's the run-7 recompute-elim class — tiny nested loop
over small arities, won't move the metric); store indices in hash_semi_join's
probe (compaction already removed those copies, ~nil gain).

## SBO targets (the run-10 lens — find hot tiny heap-vectors, inline them)
Inlining hot per-element vectors has yielded the two biggest wins of the loop:
run 10 (grounder Arguments/Achievers, +21 %) and run 12 (GroundAtom/tuple_t,
+12 %). Remaining / follow-up SBO work:
- **[DONE run 12, +12 % KEEP] GroundAtom + Table::tuple_t → small_vector<int,4>.**
  TRADEOFF: peak_mem +23 % (149→184 MB). FOLLOW-UP (next experiment): recover the
  memory — (a) try smaller inline N (2/3); (b) decouple GroundAtom (small,
  persistent in relations + packer maps) from tuple_t (wider, transient join
  width) with an element-wise convert in select_tuples. Hold the speed, cut mem.
- **JoinHashKey = std::vector<int>** (rules/join.h) — post-run-10 grounder profile
  shows `flat_hash_map<vector<int>,JoinHashEntry,VectorHash<int>>::try_emplace`
  ~6 %; the key is built+copied per fact insert. SBO candidate, contained to
  rules/join.h + a VectorHash overload for small_vector. Watch: bigger flat-map
  key slot (same memory tradeoff as run 12 — measure mem too).
- `Arguments::Arguments` copy is still ~3.7 % post-run-10 (inherent per-fact copy
  in join/product) — probably not removable without an algorithmic change.

## Strategy notes
- **Memory LAYOUT > memory ALLOCATION (run 10).** Run 4 retired malloc-shaving
  (alloc count, tcache-free); run 10 showed SBO/inline-storage is a *separate,
  live* lever — it cuts cache misses + pointer indirection on hot
  hash/compare/copy paths. When you see a tiny per-element heap vector on a
  dominant path, try SBO before concluding "allocs don't matter."
- Go structural: cut WORK (hashing, probing, copying, redundant fixpoint) and
  cut INDIRECTION (SBO), not just malloc calls.
- Noise floor ≈ 5–9 %. A real win must clear that. BATCH related cuts OR make
  one ≥10 % structural change. Confirm every would-be KEEP with a
  contemporaneous A/B (see protocol in autoresearch.md), not stored samples.

## Dead ends
- **run 3 (DISCARD)** — `reached_facts`/`newfacts` as reused members
  (retain buckets across ground()) + hoist of the join inner-loop
  head-position lookup. No gain over run 2 (median 72.4 vs 67.0) and +6 MB
  peak. Lesson: once the flat-vector lookup + join-key buffer + clean_up
  clear are in, the per-state set/vector allocations and the inner-loop
  lookup are no longer dominant. The flat-vector lookup already made the
  hoist (idea 2) nearly worthless — drop it.
