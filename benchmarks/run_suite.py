#!/usr/bin/env python3
"""Run the autoresearch benchmark suite: every (config, instance) pair, serially.

Per-task limits: 300 s wall-clock and 8 GiB of memory, both for the
translator and for the search component. No parallelism — one planner
execution at a time.

The translator output depends only on (domain, problem), so .lifted files are
cached under benchmarks/.cache/<translator-hash>/ and shared across configs
and repetitions. The hash covers the *built* translator sources, so editing
the translator invalidates the cache automatically.

The primary metric is the sum of the search component's own "Total time"
over all pairs (translation and process startup are excluded).

Behavior gate (--reference): expansions, generations, plan cost, and exit
code of every pair must match the stored reference exactly. Any mismatch
exits with code 3. This enforces behavior-preserving changes only.

Exit codes: 0 ok, 2 a pair crashed/timed out/out-of-memory, 3 behavior
mismatch against the reference.
"""

import argparse
import hashlib
import json
import os
import re
import resource
import subprocess
import sys
import tempfile

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(REPO, 'builds', 'release')
SUITE = os.path.join(REPO, 'benchmarks', 'suite.json')
CACHE = os.path.join(REPO, 'benchmarks', '.cache')

TIME_LIMIT = 300          # seconds, per task (default; see --time-limit)
MEMORY_LIMIT = 8 * 1024**3  # 8 GiB, per task

# Matches the reference experiment (2026-04-24-A-powerlifted-thesis-simul):
# every configuration translates with unit costs.
TRANSLATE_FLAGS = ['--unit-cost']

RE_TOTAL_TIME = re.compile(r'^Total time: ([0-9.e+-]+)', re.M)
RE_COST = re.compile(r'^Total plan cost: (\d+)', re.M)
RE_EXPANDED = re.compile(r'^Expanded (\d+) state', re.M)
RE_GENERATED = re.compile(r'^Generated (\d+) state', re.M)
RE_PEAK_MEM = re.compile(r'^Peak memory usage: (\d+) kB', re.M)


def set_limits():
    resource.setrlimit(resource.RLIMIT_AS, (MEMORY_LIMIT, MEMORY_LIMIT))


def run_limited(cmd, time_limit):
    """Run cmd with the per-task time and memory limits. Returns
    (exit_code, stdout) where exit_code is -1 on timeout."""
    try:
        proc = subprocess.run(cmd, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True,
                              timeout=time_limit, preexec_fn=set_limits)
        return proc.returncode, proc.stdout
    except subprocess.TimeoutExpired as e:
        out = e.stdout or b''
        if isinstance(out, bytes):  # TimeoutExpired ignores text=True
            out = out.decode(errors='replace')
        return -1, out


def translator_hash():
    """Hash the built translator sources (and the translate flags) so cache
    entries go stale when the translator changes."""
    h = hashlib.sha256()
    h.update(' '.join(TRANSLATE_FLAGS).encode())
    tdir = os.path.join(BUILD, 'translator')
    for root, _, files in sorted(os.walk(tdir)):
        for fname in sorted(files):
            if fname.endswith('.py'):
                path = os.path.join(root, fname)
                h.update(os.path.relpath(path, tdir).encode())
                with open(path, 'rb') as f:
                    h.update(f.read())
    return h.hexdigest()[:12]


def translate(domain, problem, cache_dir, time_limit):
    """Translate (domain, problem) into the cache; return the .lifted path."""
    out = os.path.join(cache_dir, f'{domain}__{problem}.lifted')
    if os.path.exists(out):
        return out
    cmd = [os.path.join(BUILD, 'translator', 'translate.py'),
           os.path.join(REPO, 'benchmarks', 'htg', domain, 'domain.pddl'),
           os.path.join(REPO, 'benchmarks', 'htg', domain, problem),
           '--output-file', out] + TRANSLATE_FLAGS
    code, output = run_limited(cmd, time_limit)
    if code != 0:
        if os.path.exists(out):
            os.remove(out)
        sys.stderr.write(output)
        raise RuntimeError(f'Translation failed for {domain}/{problem} '
                           f'(exit {code})')
    return out


def run_pair(entry, lifted, plan_dir, time_limit):
    plan_file = os.path.join(plan_dir, 'plan')
    cmd = [os.path.join(BUILD, 'search', 'search'),
           '-f', lifted,
           '-s', entry['search'],
           '-e', entry['evaluator'],
           '-g', entry['generator'],
           '--seed', '0',
           '--plan-file', plan_file]
    code, output = run_limited(cmd, time_limit)

    def grab(regex, cast):
        m = regex.search(output)
        return cast(m.group(1)) if m else None

    return {
        'config': entry['config'],
        'domain': entry['domain'],
        'problem': entry['problem'],
        'exit_code': code,
        'solved': int('Solution found.' in output),
        'search_time': grab(RE_TOTAL_TIME, float),
        'cost': grab(RE_COST, int),
        'expansions': grab(RE_EXPANDED, int),
        'generations': grab(RE_GENERATED, int),
        'peak_mem_kb': grab(RE_PEAK_MEM, int),
    }


BEHAVIOR_KEYS = ('exit_code', 'solved', 'cost', 'expansions', 'generations')


def behavior_mismatches(results, reference):
    ref = {(r['config'], r['domain'], r['problem']): r for r in reference}
    bad = []
    for r in results:
        key = (r['config'], r['domain'], r['problem'])
        if key not in ref:
            bad.append((key, 'missing from reference'))
            continue
        for field in BEHAVIOR_KEYS:
            if r[field] != ref[key][field]:
                bad.append((key, f'{field}: {ref[key][field]} -> {r[field]}'))
    return bad


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--suite', default=SUITE)
    parser.add_argument('--out', help='Write the per-pair JSON report here.')
    parser.add_argument('--reference',
                        help='Behavior reference JSON; mismatch exits 3.')
    parser.add_argument('--save-reference',
                        help='Write a behavior reference JSON here.')
    parser.add_argument('--time-limit', type=int, default=TIME_LIMIT,
                        help='Per-task wall-clock limit in seconds.')
    args = parser.parse_args()

    with open(args.suite) as f:
        suite = json.load(f)

    cache_dir = os.path.join(CACHE, translator_hash())
    os.makedirs(cache_dir, exist_ok=True)

    # Phase 1: translate every unique task (cached). Off the metric clock.
    for domain, problem in sorted({(e['domain'], e['problem']) for e in suite}):
        translate(domain, problem, cache_dir, args.time_limit)

    # Phase 2: run every pair, strictly one at a time.
    results = []
    failed = []
    with tempfile.TemporaryDirectory(prefix='pwl-suite-') as plan_dir:
        for entry in suite:
            lifted = os.path.join(
                cache_dir, f"{entry['domain']}__{entry['problem']}.lifted")
            r = run_pair(entry, lifted, plan_dir, args.time_limit)
            results.append(r)
            tag = f"{r['config']} {r['domain']}/{r['problem']}"
            if r['solved'] and r['search_time'] is not None:
                print(f'  ok   {tag}  t={r["search_time"]:.3f}s '
                      f'exp={r["expansions"]}', flush=True)
            else:
                failed.append(tag)
                print(f'  FAIL {tag}  exit={r["exit_code"]}', flush=True)

    total_time = sum(r['search_time'] for r in results
                     if r['search_time'] is not None)
    peak_mem = max((r['peak_mem_kb'] or 0) for r in results)

    if args.out:
        with open(args.out, 'w') as f:
            json.dump(results, f, indent=1)
    if args.save_reference:
        with open(args.save_reference, 'w') as f:
            json.dump([{k: r[k] for k in
                        ('config', 'domain', 'problem') + BEHAVIOR_KEYS}
                       for r in results], f, indent=1)

    print(f'METRIC search_time={total_time:.3f}')
    print(f'METRIC peak_mem_mb={peak_mem // 1024}')

    if failed:
        print(f'{len(failed)} pair(s) failed: {failed}', file=sys.stderr)
        return 2

    if args.reference:
        with open(args.reference) as f:
            reference = json.load(f)
        bad = behavior_mismatches(results, reference)
        if bad:
            for key, why in bad:
                print(f'BEHAVIOR MISMATCH {key}: {why}', file=sys.stderr)
            return 3

    return 0


if __name__ == '__main__':
    sys.exit(main())
