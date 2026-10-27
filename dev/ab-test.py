#! /usr/bin/env python3
# -*- coding: utf-8 -*-

"""
A/B benchmarking script for Powerlifted.

Runs one or more planner configurations on a set of PDDL instances under a
time and memory limit, with at most JOBS planner runs in parallel. Each
configuration is a planner checkout (with a release build) plus extra
arguments for powerlifted.py. Runs of different configurations on the same
instance are interleaved so that both see the same machine load.

Examples:

  # Run configurations A and B on all p01-p05 instances and compare:
  dev/ab-test.py --config A=/path/to/baseline --config B=. \\
      --instances 'benchmarks/*/p0[1-5].pddl' --out results.json

  # Compare two stored result files:
  dev/ab-test.py --compare a.json b.json
"""

import argparse
import glob
import json
import os
import re
import resource
import shlex
import signal
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

BASEDIR = Path(__file__).resolve().parent.parent


def parse_config(spec):
    """NAME=ROOT[:ARGS] where ARGS are extra powerlifted.py arguments.
    Arguments of the form ENV:VAR=VALUE set environment variables."""
    name, rest = spec.split('=', 1)
    if ':' in rest:
        root, args = rest.split(':', 1)
    else:
        root, args = rest, ''
    tokens = shlex.split(args)
    env = dict(t[4:].split('=', 1) for t in tokens if t.startswith('ENV:'))
    tokens = [t for t in tokens if not t.startswith('ENV:')]
    return {'name': name, 'root': str(Path(root).resolve()), 'args': tokens, 'env': env}


def find_domain(instance):
    dirname, basename = os.path.split(instance)
    for cand in ['domain.pddl', basename[:3] + '-domain.pddl',
                 'domain_' + basename, 'domain-' + basename]:
        path = os.path.join(dirname, cand)
        if os.path.exists(path):
            return path
    return None


def set_limits(mem_mb):
    def fn():
        os.setsid()
        limit = mem_mb * 1024 * 1024
        resource.setrlimit(resource.RLIMIT_AS, (limit, limit))
    return fn


def parse_output(out):
    res = {}
    m = re.search(r'Total time: ([\d.e+-]+)', out)
    if m:
        res['search_time'] = float(m.group(1))
    m = re.search(r'(?:Total plan cost|Plan cost): (\d+)', out)
    if m:
        res['cost'] = int(m.group(1))
    m = re.search(r'Plan length: (\d+)', out)
    if m:
        res['length'] = int(m.group(1))
    m = re.findall(r'Expanded (\d+) state', out)
    if m:
        res['expanded'] = int(m[-1])
    m = re.findall(r'Evaluated (\d+) state', out)
    if m:
        res['evaluated'] = int(m[-1])
    m = re.search(r'Total translation time: ([\d.]+)', out)
    if m:
        res['translate_time'] = float(m.group(1))
    return res


def run_one(config, instance, time_limit, mem_mb, validate, keep_logs):
    domain = find_domain(instance)
    with tempfile.TemporaryDirectory(prefix='pwl-ab-') as tmp:
        plan_file = os.path.join(tmp, 'plan')
        fd_driver = os.path.join(config['root'], 'fast-downward.py')
        if os.path.exists(fd_driver):
            # Fast Downward-based planner (e.g., Scorpion): ARGS are driver
            # options placed before the domain and problem files.
            cmd = ['/usr/bin/time', '-f', 'MAXRSS %M', sys.executable, fd_driver,
                   '--plan-file', plan_file] + config['args'] + [domain, instance]
        else:
            cmd = ['/usr/bin/time', '-f', 'MAXRSS %M',
                   sys.executable, os.path.join(config['root'], 'powerlifted.py'),
                   '-d', domain, '-i', instance,
                   '--translator-output-file', os.path.join(tmp, 'output.lifted'),
                   '--plan-file', plan_file] + config['args']
        start = time.time()
        env = dict(os.environ)
        env.update(config.get('env', {}))
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                cwd=tmp, preexec_fn=set_limits(mem_mb), text=True,
                                errors='replace', env=env)
        timed_out = False
        try:
            out, _ = proc.communicate(timeout=time_limit)
        except subprocess.TimeoutExpired:
            timed_out = True
            os.killpg(proc.pid, signal.SIGKILL)
            out, _ = proc.communicate()
        wall = time.time() - start
        res = parse_output(out)
        res['wall'] = round(wall, 3)
        res['timeout'] = timed_out
        res['returncode'] = proc.returncode
        m = re.findall(r'MAXRSS (\d+)', out)
        if m:
            res['memory_mb'] = round(int(m[-1]) / 1024, 1)
        plans = sorted(glob.glob(plan_file + '*'))
        res['solved'] = (not timed_out) and len(plans) > 0 and 'cost' in res
        if res['solved'] and validate:
            v = subprocess.run(['validate', domain, instance, plans[-1]],
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            res['valid'] = 'Plan valid' in v.stdout
            if not res['valid']:
                res['solved'] = False
        if keep_logs or not res['solved']:
            res['log_tail'] = out[-3000:]
        return res


def instance_key(instance):
    p = Path(instance)
    return f'{p.parent.name}/{p.name}'


def run(args):
    configs = [parse_config(c) for c in args.config]
    instances = []
    for pattern in args.instances:
        if os.path.isfile(pattern) and not pattern.endswith('.pddl'):
            with open(pattern) as f:
                for line in f:
                    line = line.strip()
                    if line:
                        instances.extend(sorted(glob.glob(line)))
        else:
            instances.extend(sorted(glob.glob(pattern)))
    instances = [i for i in instances if 'domain' not in os.path.basename(i)]
    instances = [str(Path(i).resolve()) for i in instances]
    print(f'{len(instances)} instances, {len(configs)} configs', flush=True)

    results = {c['name']: {} for c in configs}
    if args.out and os.path.exists(args.out) and args.resume:
        with open(args.out) as f:
            results.update(json.load(f)['results'])

    jobs = [(c, i) for i in instances for c in configs
            if instance_key(i) not in results.get(c['name'], {})]

    def work(job):
        c, i = job
        r = run_one(c, i, args.time, args.memory, args.validate, args.keep_logs)
        return c, i, r

    with ThreadPoolExecutor(max_workers=args.jobs) as ex:
        for c, i, r in ex.map(work, jobs):
            results[c['name']][instance_key(i)] = r
            status = 'OK ' if r['solved'] else ('TO ' if r['timeout'] else 'ERR')
            print(f"[{c['name']}] {status} {instance_key(i):45s} wall={r['wall']:7.2f}s "
                  f"mem={r.get('memory_mb', 0):7.1f}MB exp={r.get('expanded', '-')} "
                  f"cost={r.get('cost', '-')}", flush=True)
            if args.out:
                with open(args.out, 'w') as f:
                    json.dump({'configs': configs, 'results': results}, f, indent=1)
    summarize(results)


def summarize(results):
    names = list(results.keys())
    keys = sorted(set().union(*[results[n].keys() for n in names]))
    domains = sorted(set(k.split('/')[0] for k in keys))
    print()
    header = f"{'domain':32s}" + ''.join(f'{n:>12s}' for n in names)
    print('Coverage')
    print(header)
    for d in domains:
        row = f'{d:32s}'
        for n in names:
            row += f"{sum(1 for k in keys if k.startswith(d + '/') and results[n].get(k, {}).get('solved')):12d}"
        print(row)
    print(f"{'TOTAL':32s}" + ''.join(
        f"{sum(1 for k in keys if results[n].get(k, {}).get('solved')):12d}" for n in names))

    # Commonly solved: compare wall time, memory, and expansions
    common = [k for k in keys if all(results[n].get(k, {}).get('solved') for n in names)]
    if common and len(names) >= 2:
        print(f'\nOn {len(common)} commonly solved tasks (sum / geometric mean):')
        import math
        for metric in ['wall', 'memory_mb', 'expanded']:
            row = f'{metric:32s}'
            for n in names:
                vals = [max(results[n][k].get(metric, 0), 0.01) for k in common]
                gm = math.exp(sum(math.log(v) for v in vals) / len(vals))
                row += f'{sum(vals):>14.1f}/{gm:<10.2f}'
            print(row)

    # Per-domain score: IPC agile-like score (1 if <=1s, 0 if >= limit, log scale)
    import math

    def agile(r):
        if not r.get('solved'):
            return 0.0
        t = r['wall']
        if t <= 1:
            return 1.0
        return max(0.0, 1 - math.log(t) / math.log(120))
    print('\nAgile score (log-scale on wall time, 120s)')
    print(header)
    for d in domains:
        row = f'{d:32s}'
        for n in names:
            row += f"{sum(agile(results[n].get(k, {})) for k in keys if k.startswith(d + '/')):12.2f}"
        print(row)
    print(f"{'TOTAL':32s}" + ''.join(
        f"{sum(agile(results[n].get(k, {})) for k in keys):12.2f}" for n in names))


def compare(files):
    results = {}
    for f in files:
        with open(f) as fh:
            data = json.load(fh)
        for name, res in data['results'].items():
            label = name if name not in results else f'{name}@{Path(f).stem}'
            results[label] = res
    summarize(results)


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--config', action='append', default=[],
                   help='NAME=ROOT[:EXTRA ARGS]; can be passed several times')
    p.add_argument('--instances', nargs='+', default=[],
                   help='glob patterns of instances or a file with one glob per line')
    p.add_argument('--time', type=int, default=120)
    p.add_argument('--memory', type=int, default=4096, help='memory limit in MiB')
    p.add_argument('--jobs', type=int, default=2)
    p.add_argument('--out', default=None)
    p.add_argument('--resume', action='store_true')
    p.add_argument('--validate', action='store_true')
    p.add_argument('--keep-logs', action='store_true')
    p.add_argument('--compare', nargs='+', default=None)
    args = p.parse_args()
    assert args.jobs <= 2, 'never run more than two problems in parallel'
    assert args.time <= 120 and args.memory <= 4096, 'resource limits exceeded'
    if args.compare:
        compare(args.compare)
    else:
        run(args)


if __name__ == '__main__':
    main()
