#!/usr/bin/env bash
# Benchmark harness for the autoresearch loop.
#
# Rebuilds the planner, runs a fast sanity check, one discarded warm-up sweep,
# then $REPS timed sweeps over the full suite (benchmarks/suite.json).
# Prints one "METRIC search_time=<seconds>" line per timed repetition.
#
# Every sweep is behavior-gated against benchmarks/reference.json: expansions,
# generations, plan costs, and exit codes must match exactly (behavior-
# preserving changes only). Any mismatch, crash, timeout (300 s/task), or
# out-of-memory (8 GiB/task) makes this script exit non-zero.
#
# Strictly serial: at most one planner execution runs at any time.
set -euo pipefail
cd "$(dirname "$0")"

REPS=${REPS:-3}
CORE=${CORE:-2}   # physical core the timed sweeps are pinned to

# 1. Rebuild (ccache keeps this fast). A build failure is a crash.
if ! python3 build.py >/tmp/autoresearch-build.log 2>&1; then
    echo "BUILD FAILED (see /tmp/autoresearch-build.log)" >&2
    exit 2
fi

# 2. Fast sanity pre-check: one tiny instance end-to-end, expected cost 13.
./builds/release/translator/translate.py \
    dev/domains/gripper/domain.pddl dev/domains/gripper/prob01.pddl \
    --output-file /tmp/autoresearch-sanity.lifted >/dev/null 2>&1
sanity_out=$(./builds/release/search/search -f /tmp/autoresearch-sanity.lifted \
    -s gbfs -e ff -g full_reducer --seed 0 \
    --plan-file /tmp/autoresearch-sanity-plan 2>&1) || {
    echo "SANITY CHECK CRASHED" >&2; exit 2; }
if ! echo "$sanity_out" | grep -q "Total plan cost: 13"; then
    echo "SANITY CHECK FAILED (gripper prob01 cost != 13)" >&2
    exit 2
fi

# 3. Warm-up sweep, discarded (also refills the translator cache if needed).
echo "warm-up sweep..." >&2
taskset -c "$CORE" python3 benchmarks/run_suite.py \
    --reference benchmarks/reference.json >/dev/null

# 4. Timed repetitions, each behavior-gated against the reference.
#    Per-pair reports land in autoresearch-data/sweep-<rep>.json (overwritten
#    every invocation) for digging into where a delta comes from.
mkdir -p autoresearch-data
for rep in $(seq "$REPS"); do
    echo "timed sweep $rep/$REPS..." >&2
    taskset -c "$CORE" python3 benchmarks/run_suite.py \
        --reference benchmarks/reference.json \
        --out "autoresearch-data/sweep-$rep.json" | grep '^METRIC '
done
