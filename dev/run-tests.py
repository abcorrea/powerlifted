#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import json
import os
import subprocess
import sys
import timeit
from dataclasses import dataclass
from typing import Optional

from itertools import product
from pathlib import Path

"""
This test component checks that plans found in several different domains
and configurations have the expected optimal cost, and optionally tracks
performance metrics (wall time, peak memory) per test.

The expected run time of each run is less than 30s.
"""

BASEDIR = Path(__file__).resolve().parent.parent

# Commented instances have costs which are currently ignored
OPTIMAL_PLAN_COSTS = {'domains/airport/p05-airport2-p1.pddl': 21,
                      'domains/blocks/probBLOCKS-4-0.pddl': 6,
                      'domains/gripper/prob01.pddl': 11,
                      'domains/movie/prob30.pddl': 7,
                      'domains/openstacks/p01.pddl': 2,
                      'domains/organic-synthesis/p05.pddl': 2}
SEARCH_CONFIGS = ['bfs', 'gbfs']
HEURISTIC_CONFIGS = ['blind']
GENERATOR_CONFIGS = ['full_reducer', 'join', 'yannakakis']

SPECIAL_PLAN_TESTS = [
    {
        'instance': 'domains/blocks/probBLOCKS-4-0.pddl',
        'label': 'probBLOCKS-4-0-clique',
        'cost': 6,
        'validate': True,
        'configs': [('bfs', 'blind', 'clique_bk'),
                    ('bfs', 'blind', 'clique_kckp')],
    },
    {
        'instance': 'domains/object-creation/prob01.pddl',
        'label': 'object-creation-prob01',
        'cost': 1,
        'validate': False,
        'configs': [('bfs', 'blind', 'full_reducer')],
    },
]

CLI_OPTION_TESTS = [
    {
        'name': 'invalid-option',
        'args': ['--foo', 'bar', '-s', 'bfs', '-e', 'blind', '-g', 'join', '-f', 'missing'],
        'expected_code': 1,
        'expected_text': "Unknown option '--foo'.",
    },
    {
        'name': 'invalid-seed',
        'args': ['--seed', 'foo', '-s', 'bfs', '-e', 'blind', '-g', 'join', '-f', 'missing'],
        'expected_code': 1,
        'expected_text': "Invalid unsigned integer value 'foo' for option --seed.",
    },
    {
        'name': 'bool-inline-value',
        'args': ['--only-effects-novelty-check=0',
                 '--novelty-early-stop=false',
                 '-s', 'bfs',
                 '-e', 'blind',
                 '-g', 'join',
                 '-f', 'missing'],
        'expected_code': 255,
        'expected_text': 'Error opening the task file: missing',
    },
]


@dataclass
class TestResult:
    name: str
    domain: str
    instance_name: str
    config: str
    passed: bool
    wall_time: float
    peak_memory_kb: Optional[int]
    expected_cost: Optional[int]
    found_cost: Optional[int]
    plan_valid: Optional[bool]


class TestRun:
    def __init__(self, instance, config, validate=True, label=None):
        self.instance = instance
        self.search = config[0]
        self.heuristic = config[1]
        self.generator = config[2]
        self.validate = validate
        self.label = label

    def get_config(self):
        return "{}, {}, and {}".format(self.search,
                                       self.heuristic,
                                       self.generator)

    @property
    def name(self):
        if self.label is not None:
            instance_short = self.label
        else:
            instance_short = Path(self.instance).stem
        return "{}[{},{},{}]".format(instance_short, self.search,
                                     self.heuristic, self.generator)

    @property
    def domain(self):
        """Extract domain name from path like 'domains/blocks/prob.pddl'."""
        return Path(self.instance).parts[1]

    @property
    def instance_name(self):
        return Path(self.instance).stem

    @property
    def config(self):
        return "{},{},{}".format(self.search, self.heuristic, self.generator)

    def __str__(self):
        return "{} with {}".format(self.instance, self.get_config())

    def run(self):
        """Run the planner and return (output_bytes, wall_time_s, peak_memory_kb)."""
        cmd = [str(BASEDIR / 'powerlifted.py'),
               '-i', str(BASEDIR / 'dev' / self.instance),
               '-s', self.search,
               '-e', self.heuristic,
               '-g', self.generator]
        if self.validate:
            cmd.append('--validate')

        start = timeit.default_timer()
        # Use GNU time to capture peak RSS if available
        time_bin = '/usr/bin/time'
        peak_kb = None
        if os.path.isfile(time_bin):
            time_output_file = '/tmp/powerlifted_test_time.tmp'
            full_cmd = [time_bin, '-v', '-o', time_output_file] + cmd
            output = subprocess.check_output(full_cmd, stderr=subprocess.STDOUT)
            wall_time = timeit.default_timer() - start
            try:
                with open(time_output_file) as f:
                    for line in f:
                        if 'Maximum resident set size' in line:
                            peak_kb = int(line.strip().split()[-1])
            except (OSError, ValueError):
                pass
        else:
            output = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
            wall_time = timeit.default_timer() - start

        return output, wall_time, peak_kb

    def evaluate(self, output, optimal_cost, wall_time, peak_kb):
        """Evaluate output and return a TestResult."""
        plan_length_found = None
        plan_valid = None
        for line in output.splitlines():
            if b'Total plan cost:' in line:
                plan_length_found = int(line.split()[3])
            if b'Plan valid' in line:
                plan_valid = True

        passed = (plan_length_found == optimal_cost)
        if self.validate and not plan_valid:
            passed = False
        elif plan_valid is False:
            passed = False

        status = "PASSED" if passed else "FAILED"
        details = ""
        if not passed:
            if plan_length_found != optimal_cost:
                details += " [expected: {}, found: {}]".format(
                    optimal_cost, plan_length_found)
            if self.validate and not plan_valid:
                details += " [plan was not validated]"
            elif plan_valid is False:
                details += " [VAL did not validate the plan]"

        time_str = "{:.2f}s".format(wall_time)
        mem_str = "{:.1f}MB".format(peak_kb / 1024) if peak_kb else "N/A"
        print("{} {} (time: {}, mem: {}){}".format(
            status, self, time_str, mem_str, details))

        return TestResult(
            name=self.name,
            domain=self.domain,
            instance_name=self.instance_name,
            config=self.config,
            passed=passed,
            wall_time=wall_time,
            peak_memory_kb=peak_kb,
            expected_cost=optimal_cost,
            found_cost=plan_length_found,
            plan_valid=plan_valid,
        )

    def remove_plan_file(self):
        for plan_file in ('sas_plan', 'plan'):
            if os.path.isfile(plan_file):
                os.remove(plan_file)
        for plan_path in Path('.').glob('plan.*'):
            if plan_path.is_file():
                plan_path.unlink()


def run_cli_option_test(test):
    search_binary = BASEDIR / 'builds' / 'release' / 'search' / 'search'
    cmd = [str(search_binary)] + test['args']
    start = timeit.default_timer()
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
    wall_time = timeit.default_timer() - start
    output = proc.stdout.decode('utf-8', errors='replace')

    passed = proc.returncode == test['expected_code'] and test['expected_text'] in output
    status = 'PASSED' if passed else 'FAILED'
    details = ''
    if proc.returncode != test['expected_code']:
        details += ' [expected exit: {}, found: {}]'.format(
            test['expected_code'], proc.returncode)
    if test['expected_text'] not in output:
        details += " [missing output: '{}']".format(test['expected_text'])

    print("{} cli option test '{}' (time: {:.2f}s){}".format(
        status, test['name'], wall_time, details))

    return TestResult(
        name='cli-option-{}'.format(test['name']),
        domain='cli',
        instance_name=test['name'],
        config='search-options',
        passed=passed,
        wall_time=wall_time,
        peak_memory_kb=None,
        expected_cost=None,
        found_cost=None,
        plan_valid=None,
    )


def print_summary_table(results, total_time):
    """Print a formatted summary table of all test results."""
    w = 100
    print("\n" + "=" * w)
    print("SUMMARY")
    print("=" * w)

    # Header
    print("{:<18} {:<25} {:<28} {:>6} {:>8} {:>8}".format(
        "Domain", "Instance", "Config", "Status", "Time", "Memory"))
    print("-" * w)

    for r in results:
        status = "OK" if r.passed else "FAIL"
        time_str = "{:.2f}s".format(r.wall_time)
        mem_str = "{:.1f}MB".format(r.peak_memory_kb / 1024) if r.peak_memory_kb else "N/A"
        print("{:<18} {:<25} {:<28} {:>6} {:>8} {:>8}".format(
            r.domain[:18], r.instance_name[:25], r.config[:28],
            status, time_str, mem_str))

    print("-" * w)
    passes = sum(1 for r in results if r.passed)
    failures = sum(1 for r in results if not r.passed)
    total = passes + failures
    times = [r.wall_time for r in results]
    print("Passed: {}/{}  |  Total time: {:.2f}s  |  "
          "Slowest: {:.2f}s  |  Mean: {:.2f}s".format(
              passes, total, total_time,
              max(times) if times else 0,
              sum(times) / len(times) if times else 0))
    if failures > 0:
        print("FAILED tests:")
        for r in results:
            if not r.passed:
                print("  - {}".format(r.name))
    print("=" * w)


def store_results(results, filepath):
    """Save timing results to a JSON file."""
    data = {}
    for r in results:
        data[r.name] = {
            'wall_time': r.wall_time,
            'peak_memory_kb': r.peak_memory_kb,
            'passed': r.passed,
        }
    with open(filepath, 'w') as f:
        json.dump(data, f, indent=2)
    print("Results saved to {}".format(filepath))


def compare_results(results, filepath):
    """Compare current results against previously saved results."""
    try:
        with open(filepath) as f:
            previous = json.load(f)
    except (OSError, json.JSONDecodeError) as e:
        print("Could not load results {}: {}".format(filepath, e))
        return

    w = 100
    print("\n" + "=" * w)
    print("COMPARISON vs: {}".format(filepath))
    print("=" * w)
    print("{:<18} {:<25} {:<28} {:>10} {:>10} {:>10}".format(
        "Domain", "Instance", "Config", "Previous", "Current", "Change"))
    print("-" * w)

    for r in results:
        if r.name not in previous:
            print("{:<18} {:<25} {:<28} {:>10} {:>10} {:>10}".format(
                r.domain[:18], r.instance_name[:25], r.config[:28],
                "N/A", "{:.2f}s".format(r.wall_time), "new"))
            continue
        b = previous[r.name]
        b_time = b['wall_time']
        change = (r.wall_time - b_time) / b_time * 100 if b_time > 0 else 0
        marker = ""
        if change > 20:
            marker = " SLOWER"
        elif change < -20:
            marker = " FASTER"
        print("{:<18} {:<25} {:<28} {:>10} {:>10} {:>9.0f}%{}".format(
            r.domain[:18], r.instance_name[:25], r.config[:28],
            "{:.2f}s".format(b_time),
            "{:.2f}s".format(r.wall_time),
            change,
            marker))

    print("=" * w)


def parse_options():
    parser = argparse.ArgumentParser(
        description='Run regression tests for the Powerlifted planner.')
    parser.add_argument('--minimal', action='store_true',
                        help='Use minimal test set.')
    parser.add_argument('--store-results', metavar='FILE',
                        help='Save timing results to a JSON file.')
    parser.add_argument('--compare-results', metavar='FILE',
                        help='Compare results against a previously saved JSON file.')
    return parser.parse_args()


if __name__ == '__main__':
    args = parse_options()

    plan_costs = dict(OPTIMAL_PLAN_COSTS)
    search_configs = list(SEARCH_CONFIGS)
    heuristic_configs = list(HEURISTIC_CONFIGS)
    generator_configs = list(GENERATOR_CONFIGS)

    if args.minimal:
        plan_costs = {'domains/blocks/probBLOCKS-4-0.pddl': 6,
                      'domains/gripper/prob01.pddl': 11,
                      'domains/movie/prob30.pddl': 7}
        search_configs = ['bfs', 'gbfs']
        heuristic_configs = ['blind']
        generator_configs = ['full_reducer', 'yannakakis']

    start = timeit.default_timer()
    results = []

    for instance, cost in plan_costs.items():
        for config in product(search_configs, heuristic_configs, generator_configs):
            test = TestRun(instance, config)
            try:
                output, wall_time, peak_kb = test.run()
                result = test.evaluate(output, cost, wall_time, peak_kb)
            except subprocess.CalledProcessError as e:
                print("FAILED {} (process exited with code {})".format(
                    test, e.returncode))
                result = TestResult(
                    name=test.name, domain=test.domain,
                    instance_name=test.instance_name, config=test.config,
                    passed=False, wall_time=0, peak_memory_kb=None,
                    expected_cost=cost, found_cost=None, plan_valid=None)
            results.append(result)
            test.remove_plan_file()

    for test_case in SPECIAL_PLAN_TESTS:
        for config in test_case['configs']:
            test = TestRun(test_case['instance'],
                           config,
                           validate=test_case['validate'],
                           label=test_case['label'])
            try:
                output, wall_time, peak_kb = test.run()
                result = test.evaluate(output, test_case['cost'], wall_time, peak_kb)
            except subprocess.CalledProcessError as e:
                print("FAILED {} (process exited with code {})".format(
                    test, e.returncode))
                result = TestResult(
                    name=test.name,
                    domain=test.domain,
                    instance_name=test.instance_name,
                    config=test.config,
                    passed=False,
                    wall_time=0,
                    peak_memory_kb=None,
                    expected_cost=test_case['cost'],
                    found_cost=None,
                    plan_valid=None)
            results.append(result)
            test.remove_plan_file()

    for cli_test in CLI_OPTION_TESTS:
        results.append(run_cli_option_test(cli_test))

    total_time = timeit.default_timer() - start
    print_summary_table(results, total_time)

    if args.store_results:
        store_results(results, args.store_results)

    if args.compare_results:
        compare_results(results, args.compare_results)

    failures = sum(1 for r in results if not r.passed)
    sys.exit(1 if failures > 0 else 0)
