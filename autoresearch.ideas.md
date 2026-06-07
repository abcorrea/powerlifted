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

1. [TRYING] `get_head_position_of_arg`: replace unordered_map<Term,int> with a
   flat vector<pair<Term,int>> + single linear scan (heads have 2–4 args).
   Kills the double lookup. Behavior-identical (dedup keeps "last wins").
2. Hoist `get_head_position_of_arg` out of join's inner loop over matched
   facts — the (cond-pos → head-pos) mapping is loop-invariant. Precompute
   once per join() call.
3. JoinHashKey `key` vector: reuse a member buffer instead of allocating a
   fresh vector (reserve) every join() call.
4. `JoinRule::clean_up` rebuilds two phmap maps via `= JoinHashTable()` each
   ground(); `.clear()` instead to retain bucket arrays (fewer mallocs).
5. `is_cheapest_path_to_achieve_fact`: the find()+insert() does two hashings
   of the same Fact; use a single lazy_emplace / find-then-insert-by-iterator.
6. Fact stores Arguments (vector<Term>) + Achievers (vector<int>) — lots of
   small-vector allocs. Consider small-buffer optimization for Arguments.
7. State packing / hashing (`src/search/states/`) for the successor-gen side
   (full_reducer/join/yannakakis generators, used by bfs/gbfs/bfws).

## Dead ends

(none yet)
