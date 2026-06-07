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
7. Fact stores Arguments (vector<Term>) + Achievers (vector<int>) — small-vector
   allocs. SBO for Arguments is the one allocation idea NOT yet tried, but run 4
   suggests malloc isn't the bottleneck, so deprioritize.
8. `JoinHashEntry` is a `flat_hash_set<Fact>` per key, but join only ever
   ITERATES it (never dedups on read) — so the per-key set may be doing
   pointless hashing on insert. Check whether duplicate facts per key are
   possible; if not, a `std::vector<Fact>` (or vector<int> of fact indices)
   per key would cut hashing AND the prepare_insert cost (3–7 % symbol). This
   is structural, not malloc-shaving — promising.
9. The Fact copied into the join hash entries / reached_facts carries
   Arguments+Achievers it doesn't need there (only args, cost, index are read
   from hash entries; only cost+index from reached_facts). Storing lighter
   records (e.g. fact indices) in those sets could cut copy + hash cost.
   Structural.
10. `JoinHashKey` is a `vector<int>` used as a phmap map key — hashed+compared
   on every join. For single-var joins (very common) the key is one int; a
   specialized int-keyed path could skip vector hashing. Structural.
11. State packing / hashing (`src/search/states/`) for the successor-gen side
   (full_reducer/join/yannakakis generators). Less hot than the heuristic
   grounder but in every config; separate from the grounder entirely.
12. Algorithmic: hmax/add recomputation across sibling states — the grounder
   reruns the whole Datalog fixpoint per state from scratch. Any provably-safe
   incrementality/memoization would be a big win, but must not change returned
   heuristic values (hard to keep behavior-identical). High risk/high reward.

## Strategy notes
- **Grounder malloc-shaving is exhausted (run 4).** glibc tcache makes the
  small frequent allocs near-free; cutting them doesn't move the metric. Go
  structural: cut WORK (hashing, probing, copying, redundant fixpoint), not
  malloc calls. Ideas 8–10 target the actual hashing/probing in join.
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
