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
5. `is_cheapest_path_to_achieve_fact`: the find()+insert() does two hashings
   of the same Fact; use phmap `lazy_emplace` / a single find-then-insert by
   iterator so the Fact is hashed once. (~4 % symbol.) Behavior-identical.
6. `reached_facts` set rebuilt fresh every ground() call (local in ground());
   the per-state churn could reuse a cleared member set (retain buckets), like
   the join tables. Watch behavior: it must be empty at each ground() entry.
7. Fact stores Arguments (vector<Term>) + Achievers (vector<int>) — lots of
   small-vector allocs. Consider small-buffer optimization for Arguments
   (heads/conditions are short). Bigger change; screen carefully.
8. `JoinHashEntry` is a `flat_hash_set<Fact>` per key — but join only ever
   iterates it (never dedups on read). Check whether duplicate facts per key
   are possible; if a vector suffices it'd cut hashing on insert.
9. State packing / hashing (`src/search/states/`) for the successor-gen side
   (full_reducer/join/yannakakis generators, used by bfs/gbfs/bfws). Less hot
   than the heuristic grounder but still in every config.

## Strategy notes
- Individual grounder micro-opts are below the ~5–9 % noise floor. BATCH
  2–3 related behavior-preserving cuts per experiment so the combined effect
  clears the gate (this is how run 2 passed). Screen the batch at REPS=3,
  confirm at REPS=5.
- Bigger structural changes (≥10 %) are the other way to beat the floor.

## Dead ends
- (none yet)
