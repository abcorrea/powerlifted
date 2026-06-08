# Kickstarting the autoresearch agent

Everything is already set up on this branch
(`autoresearch/search-time-2026-06-07`): benchmark suite, harness, behavior
reference, correctness checks, brain file, and a measured baseline. To start
(or resume) the loop, open a fresh Claude Code session in this directory and
paste the prompt below.

---

Run autoresearch on this repo. The setup already exists — do not recreate
it.

1. Read `autoresearch.md` (the brain file), the tail of
   `autoresearch.jsonl`, and `git log --oneline -20`, then continue from
   wherever the loop left off.
2. Loop forever: think → edit → `./autoresearch.sh` → decide with
   `.claude/skills/autoresearch/scripts/decide.py` → commit or revert →
   append one line to `autoresearch.jsonl` → repeat. Never stop to ask
   whether to continue; I will interrupt when I want to steer.
3. Hard rules, non-negotiable:
   - Behavior-preserving changes only. The harness enforces this via
     `benchmarks/reference.json`; never regenerate or edit that file, the
     suite, the harness scripts, or anything listed as off-limits in
     `autoresearch.md`.
   - No new dependencies. Only the C++ standard library and the in-tree
     vendored deps (`src/search/parallel_hashmap/`) are allowed — **no boost,
     no other third-party/system library** (boost lives in `/usr/include` so
     it compiles silently, but it is not a project dependency).
     `autoresearch.checks.sh` runs `autoresearch.guard.sh` to enforce this; a
     guard failure reverts the experiment like any check failure. If you truly
     need a container the stdlib lacks, vendor a minimal single-header version
     into the repo instead of including an external one.
   - Keep a change only when `decide.py` says KEEP; revert on DISCARD,
     crash, or a `./autoresearch.checks.sh` failure, even if the numbers
     looked good.
   - Strictly one planner execution at a time, and nothing else CPU-heavy
     while sweeps are being timed.
   - Never use `git clean`. Revert with `git checkout -- .` and remove new
     files by hand.
4. Update the **What's Been Tried** section of `autoresearch.md` every 5–10
   experiments so a future agent does not repeat dead ends.

---

## Notes

- Cold-start cost per experiment is ~7 min (rebuild + sanity + warm-up +
  3 timed sweeps), so screen ideas cheaply before measuring expensive ones.
- If the machine or workload changes (new hardware, edited suite), re-init
  `autoresearch.jsonl` with a fresh config header and re-measure the
  baseline — old samples are not comparable.
- To steer mid-run, just type; the agent folds your instruction into the
  next experiment after finishing the current one.
