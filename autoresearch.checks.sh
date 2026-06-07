#!/usr/bin/env bash
# Correctness gate for the autoresearch loop, run after a passing benchmark
# (off the metric clock). A failure here reverts the experiment like a crash.
#
# Runs the full local regression suite: plan-cost regression tests, heuristic
# smoke tests, novelty/width smoke tests, and CLI option tests.
set -euo pipefail
cd "$(dirname "$0")"

python3 dev/run-tests.py
