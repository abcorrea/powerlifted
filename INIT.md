# Kickstarting the grounder-work autoresearch agent

Everything is already set up on this branch
(`autoresearch/grounder-work-2026-06-30`): benchmark suite, harness, behavior
reference, correctness checks, brain file, the in-binary grounder meter, and a
measured baseline. To start (or resume) the loop, open a fresh Claude Code
session in this directory and paste the prompt below.

---

Run autoresearch on this repo. The setup already exists — do not recreate it.

1. Read `autoresearch.md` (the brain file), the tail of `autoresearch.jsonl`,
   and `git log --oneline -20`, then start experimenting.
2. Goal: make the Datalog grounder do **less work per state** so the add/ff
   heuristics evaluate faster. The headline lever: at baseline ~53% of grounder
   productions are wasted (produced, then discarded as already-cheaper). The hot
   path is `src/search/datalog/grounder/weighted_grounder.cc` (`join` /
   `is_cheapest_path_to_achieve_fact`); a secondary lever is partial
   common-subexpression sharing across rules in
   `src/search/datalog/transformations/`. See the idea backlog in
   `autoresearch.md`. Profile a heavy pair first.
3. Loop forever: think → edit → `./autoresearch.sh` → check the deterministic
   `grounder_atoms` counter, then confirm the `search_time` win with a
   contemporaneous A/B via `.claude/skills/autoresearch/scripts/decide.py` →
   `./autoresearch.checks.sh` → commit or revert → append one line to
   `autoresearch.jsonl` → repeat. Never stop to ask whether to continue.
4. IMPORTANT screening rule for this loop: there are two kinds of win —
   *producing fewer facts* (`grounder_atoms` drops) and *producing the same facts
   more cheaply* (`grounder_atoms` flat, time still drops). **Do NOT discard an
   idea just because `grounder_atoms` is flat** — judge wins on `search_time`.
   Use the counter to classify ("fewer" vs "cheaper"), not to gate.
5. Hard rules, non-negotiable:
   - **Relaxed behavior gate**: every instance the baseline solved must still
     solve with plan cost ≤ baseline (enforced via `benchmarks/reference.json`).
     Any optimization that drops/reorders productions **must not change a kept
     fact's final cost or its recorded best achiever** — that invariant keeps
     h^add and h^ff correct. Never regenerate `reference.json`, the suite, the
     harness, or **the meter** (`grounder_statistics.h` and its call sites).
   - No new dependencies (stdlib + in-tree `parallel_hashmap/`,
     `utils/small_vector.h` only — no boost). `autoresearch.checks.sh` runs
     `autoresearch.guard.sh` to enforce it.
   - Keep a change only on a `decide.py` KEEP from a contemporaneous A/B; revert
     on DISCARD, crash, or a `./autoresearch.checks.sh` failure.
   - Strictly one planner execution at a time; nothing else CPU-heavy (incl.
     `perf`) while sweeps are timed. Never `git clean` — revert with
     `git checkout -- .` and delete new files by hand.
6. Update **What's Been Tried** in `autoresearch.md` every 5–10 experiments.

---

## Notes

- The win available is large (~53% wasted productions), so real wins should
  comfortably clear the ~5% time-noise floor — but still confirm with a
  contemporaneous REPS=5 A/B (the box drifts ±10–15% over minutes).
- Behavior nuance: grounder-work cuts are exactly value-preserving for h^add (its
  expansions must NOT move — a trajectory note on an add pair means you broke a
  value); for h^ff the relaxed plan may shift via achiever ties, allowed as long
  as it still solves at ≤ cost.
- This loop follows the greedy-join loop (`autoresearch/greedy-join-2026-06-30`...
  branch `autoresearch/greedy-join-2026-06-29`), which proved join *ordering* is
  already well-tuned; the leverage is redundant grounder *work*. See
  `autoresearch.md` → "Why this target".
- To steer mid-run, just type; the agent folds it into the next experiment.
