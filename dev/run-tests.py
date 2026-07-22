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
    {
        # Regression test for issue #64: disjunctive preconditions are split
        # into several action schemas with the same name, and the Datalog
        # heuristics used to collapse them into a single action predicate.
        'instance': 'domains/disjunction/prob01.pddl',
        'label': 'disjunction-prob01',
        'cost': 2,
        'validate': True,
        'configs': [('gbfs', 'add', 'full_reducer'),
                    ('gbfs', 'hmax', 'full_reducer'),
                    ('gbfs', 'ff', 'full_reducer'),
                    ('gbfs', 'rff', 'full_reducer')],
    },
    {
        'instance': 'domains/blocks/probBLOCKS-4-0.pddl',
        'label': 'probBLOCKS-4-0-full-novelty',
        'cost': None,
        'validate': True,
        'required_output': ['--full-novelty-check',
                            'Total number of goal atoms:',
                            'Initial heuristic value ',
                            'Starting BFWS',
                            'Using version with R-X',
                            'Initial h-add value of the task:'],
        'configs': [('bfws1-rx', 'blind', 'full_reducer')],
        'extra_args_by_config': {
            ('bfws1-rx', 'blind', 'full_reducer'): ['--full-novelty-check'],
        },
    },
]

HEURISTIC_PLAN_TESTS = [
    {
        'instance': 'domains/blocks/probBLOCKS-4-0.pddl',
        'label': 'probBLOCKS-4-0-heuristics',
        'cost': None,
        'validate': True,
        'required_output': ['Time to evaluate initial state:', 'Initial heuristic value '],
        'configs': [('gbfs', 'goalcount', 'full_reducer'),
                    ('gbfs', 'add', 'full_reducer'),
                    ('gbfs', 'hmax', 'full_reducer'),
                    ('gbfs', 'ff', 'full_reducer'),
                    ('gbfs', 'rff', 'full_reducer')],
    },
    {
        'instance': 'domains/gripper/prob01.pddl',
        'label': 'prob01-heuristics',
        'cost': None,
        'validate': True,
        'required_output': ['Time to evaluate initial state:', 'Initial heuristic value '],
        'configs': [('gbfs', 'goalcount', 'full_reducer'),
                    ('gbfs', 'add', 'full_reducer'),
                    ('gbfs', 'hmax', 'full_reducer'),
                    ('gbfs', 'ff', 'full_reducer'),
                    ('gbfs', 'rff', 'full_reducer')],
    },
    {
        'instance': 'domains/movie/prob30.pddl',
        'label': 'prob30-heuristics',
        'cost': None,
        'validate': True,
        'required_output': ['Time to evaluate initial state:', 'Initial heuristic value '],
        'configs': [('gbfs', 'goalcount', 'full_reducer'),
                    ('gbfs', 'add', 'full_reducer'),
                    ('gbfs', 'hmax', 'full_reducer'),
                    ('gbfs', 'ff', 'full_reducer'),
                    ('gbfs', 'rff', 'full_reducer')],
    },
]

NOVELTY_PLAN_TESTS = [
    {
        'instance': 'domains/blocks/probBLOCKS-4-0.pddl',
        'label': 'probBLOCKS-4-0-novelty',
        'cost': None,
        'validate': True,
        'configs': [('bfws1', 'blind', 'full_reducer'),
                    ('bfws1-rx', 'blind', 'full_reducer'),
                    ('alt-bfws1', 'ff', 'full_reducer'),
                    ('dq-bfws1-rx', 'ff', 'full_reducer')],
        'required_output': ['Total number of goal atoms:', 'Initial heuristic value '],
        'extra_args_by_config': {
            ('bfws1-rx', 'blind', 'full_reducer'): ['--novelty-early-stop'],
        },
        'required_output_by_config': {
            ('bfws1', 'blind', 'full_reducer'): ['Starting BFWS'],
            ('bfws1-rx', 'blind', 'full_reducer'): ['Starting BFWS',
                                                    'Using version with R-X',
                                                    'Initial h-add value of the task:'],
            ('alt-bfws1', 'ff', 'full_reducer'): ['Starting AlternatedBFWS',
                                                  'Initial h-add value of the task:'],
            ('dq-bfws1-rx', 'ff', 'full_reducer'): ['Starting Dual-Queue BFWS',
                                                    'Initial h-add value of the task:'],
        },
    },
]

AXIOM_PLAN_TESTS = [
    {
        # Recursive transitive closure (reachable) with the goal being a
        # derived atom; the recursive rule has an existential body variable.
        'instance': 'domains/axioms-reachability/prob01.pddl',
        'label': 'axioms-reachability-01',
        'cost': 2,
        'validate': True,
        'configs': [('bfs', 'blind', 'join'),
                    ('bfs', 'blind', 'yannakakis'),
                    ('astar', 'blind', 'full_reducer'),
                    ('gbfs', 'goalcount', 'full_reducer')],
    },
    {
        'instance': 'domains/axioms-reachability/prob01.pddl',
        'label': 'axioms-reachability-01-heur',
        'cost': None,
        'validate': True,
        'configs': [('gbfs', 'add', 'full_reducer'),
                    ('gbfs', 'hmax', 'full_reducer'),
                    ('gbfs', 'ff', 'join'),
                    ('gbfs', 'rff', 'full_reducer'),
                    ('lazy', 'ff', 'full_reducer')],
        'required_output_by_config': {
            ('gbfs', 'add', 'full_reducer'): ['Initial heuristic value 2'],
            ('gbfs', 'hmax', 'full_reducer'): ['Initial heuristic value 1'],
            ('gbfs', 'ff', 'join'): ['Initial heuristic value 2'],
            ('gbfs', 'rff', 'full_reducer'): ['Initial heuristic value 2'],
        },
    },
    {
        # Disjunctive goal over derived atoms: the translator introduces a
        # nullary goal axiom split into several axioms with constants in
        # their bodies.
        'instance': 'domains/axioms-reachability/prob02.pddl',
        'label': 'axioms-reachability-02',
        'cost': 1,
        'validate': True,
        'configs': [('bfs', 'blind', 'join'),
                    ('gbfs', 'add', 'full_reducer'),
                    ('gbfs', 'ff', 'yannakakis')],
    },
    {
        # Recursive 'above' with the derived atom in the goal.
        'instance': 'domains/axioms-above/prob01.pddl',
        'label': 'axioms-above-01',
        'cost': 2,
        'validate': True,
        'configs': [('bfs', 'blind', 'join'),
                    ('bfs', 'blind', 'yannakakis'),
                    ('astar', 'blind', 'full_reducer')],
    },
    {
        'instance': 'domains/axioms-above/prob01.pddl',
        'label': 'axioms-above-01-heur',
        'cost': None,
        'validate': True,
        'configs': [('gbfs', 'add', 'full_reducer'),
                    ('gbfs', 'hmax', 'full_reducer'),
                    ('gbfs', 'ff', 'full_reducer'),
                    ('gbfs', 'rff', 'full_reducer')],
        'required_output': ['Initial heuristic value 2'],
    },
    {
        # Recursive 'above' used in an action precondition; the initial
        # state already satisfies it through the recursive rule.
        'instance': 'domains/axioms-above/prob02.pddl',
        'label': 'axioms-above-02',
        'cost': 1,
        'validate': True,
        'configs': [('bfs', 'blind', 'join'),
                    ('bfs', 'blind', 'full_reducer'),
                    ('gbfs', 'ff', 'yannakakis'),
                    ('gbfs', 'hmax', 'full_reducer')],
    },
    {
        # Two strata (marked-pair feeds special), an inequality literal in
        # an axiom body, and a constant in an axiom body.
        'instance': 'domains/axioms-strata/prob01.pddl',
        'label': 'axioms-strata-01',
        'cost': 3,
        'validate': True,
        'configs': [('bfs', 'blind', 'join'),
                    ('bfs', 'blind', 'yannakakis'),
                    ('astar', 'blind', 'full_reducer'),
                    ('gbfs', 'add', 'full_reducer'),
                    ('gbfs', 'ff', 'full_reducer')],
    },
]

# Exhaustive searches with a known reachable state count: derived atoms are
# a function of the base fluents, so they must not affect duplicate
# detection. prob03's reachable state space is exactly the 16 subsets of
# buildable roads; more registered states would mean states differing only
# in derived atoms coexisted.
AXIOM_UNSOLVABLE_TESTS = [
    {
        'name': 'axioms-reachability-03-duplicates',
        'instance': 'domains/axioms-reachability/prob03.pddl',
        'configs': [('bfs', 'blind', 'join'),
                    ('bfs', 'blind', 'full_reducer')],
        'required_output': ['No solution found!',
                           'Number of registered states: 16'],
    },
    {
        # Exercises delete effects: stale derived (above ...) atoms
        # surviving an unstack would raise the state count above 10.
        'name': 'axioms-above-03-stale-derived',
        'instance': 'domains/axioms-above/prob03.pddl',
        'configs': [('bfs', 'blind', 'join'),
                    ('bfs', 'blind', 'yannakakis')],
        'required_output': ['No solution found!',
                           'Number of registered states: 10'],
    },
]

# Tasks outside the supported axiom fragment: the translator must reject
# them with an informative message (and a non-zero exit code).
TRANSLATOR_REJECTION_TESTS = [
    {
        'name': 'axioms-negated-body',
        'domain': 'domains/axioms-invalid/negated-body-domain.pddl',
        'instance': 'domains/axioms-invalid/negated-body-prob01.pddl',
        'expected_text': 'contains the negated atom',
    },
    {
        'name': 'axioms-forall-body',
        'domain': 'domains/axioms-invalid/forall-body-domain.pddl',
        'instance': 'domains/axioms-invalid/forall-body-prob01.pddl',
        'expected_text': 'negation over the derived predicate',
    },
    {
        'name': 'axioms-negated-derived-precond',
        'domain': 'domains/axioms-invalid/negated-derived-precond-domain.pddl',
        'instance': 'domains/axioms-invalid/negated-derived-precond-prob01.pddl',
        'expected_text': 'negated derived predicate',
    },
    {
        'name': 'axioms-derived-in-effect',
        'domain': 'domains/axioms-invalid/derived-in-effect-domain.pddl',
        'instance': 'domains/axioms-invalid/derived-in-effect-prob01.pddl',
        'expected_text': 'appears in effect of action',
    },
    {
        'name': 'axioms-derived-in-init',
        'domain': 'domains/axioms-invalid/derived-in-init-domain.pddl',
        'instance': 'domains/axioms-invalid/derived-in-init-prob01.pddl',
        'expected_text': 'appears in :init fact',
    },
]

# The four IPC domains with derived predicates all fall outside the
# supported fragment (negation over derived predicates after normalization,
# or quantified action conditions); they must be rejected cleanly. Only run
# when the benchmarks are available.
DOWNWARD_BENCHMARKS = os.environ.get('DOWNWARD_BENCHMARKS')
IPC_AXIOM_REJECTION_TESTS = [
    {
        'name': 'ipc-philosophers',
        'domain': 'philosophers/domain.pddl',
        'instance': 'philosophers/p01-phil2.pddl',
        'expected_text': 'not supported',
    },
    {
        'name': 'ipc-optical-telegraphs',
        'domain': 'optical-telegraphs/domain.pddl',
        'instance': 'optical-telegraphs/p01-opt2.pddl',
        'expected_text': 'not supported',
    },
    {
        'name': 'ipc-psr-middle',
        'domain': 'psr-middle/domain.pddl',
        'instance': 'psr-middle/p01-s17-n2-l2-f30.pddl',
        'expected_text': 'not supported',
    },
    {
        'name': 'ipc-psr-large',
        'domain': 'psr-large/domain.pddl',
        'instance': 'psr-large/p01-s29-n2-l5-f30.pddl',
        'expected_text': 'not supported',
    },
]

UNIT_TESTS = [
    {
        'name': 'axiom-stratification',
        'cmd': [sys.executable,
                str(BASEDIR / 'src' / 'translator' / 'tests' / 'test_axiom_rules.py')],
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
        'args': ['--full-novelty-check=0',
                 '--novelty-early-stop=false',
                 '-s', 'bfs',
                 '-e', 'blind',
                 '-g', 'join',
                 '-f', 'missing'],
        'expected_code': 33,
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
    def __init__(self,
                 instance,
                 config,
                 validate=True,
                 label=None,
                 required_output=None,
                 extra_args=None):
        self.instance = instance
        self.search = config[0]
        self.heuristic = config[1]
        self.generator = config[2]
        self.validate = validate
        self.label = label
        self.required_output = required_output or []
        self.extra_args = extra_args or []

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
        cmd.extend(self.extra_args)

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
        decoded_output = output.decode('utf-8', errors='replace')
        for line in output.splitlines():
            if b'Total plan cost:' in line:
                plan_length_found = int(line.split()[3])
            if b'Plan valid' in line:
                plan_valid = True

        passed = True
        if optimal_cost is not None:
            passed = (plan_length_found == optimal_cost)
        elif plan_length_found is None:
            passed = False
        if self.validate and not plan_valid:
            passed = False
        elif plan_valid is False:
            passed = False
        for marker in self.required_output:
            if marker not in decoded_output:
                passed = False

        status = "PASSED" if passed else "FAILED"
        details = ""
        if not passed:
            if optimal_cost is not None and plan_length_found != optimal_cost:
                details += " [expected: {}, found: {}]".format(
                    optimal_cost, plan_length_found)
            elif optimal_cost is None and plan_length_found is None:
                details += " [plan cost not reported]"
            if self.validate and not plan_valid:
                details += " [plan was not validated]"
            elif plan_valid is False:
                details += " [VAL did not validate the plan]"
            for marker in self.required_output:
                if marker not in decoded_output:
                    details += " [missing output: '{}']".format(marker)

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


def run_translator_rejection_test(test, base_dir):
    """Run the translator alone and expect a clean rejection."""
    translator = BASEDIR / 'src' / 'translator' / 'translate.py'
    output_file = '/tmp/powerlifted_test_rejection.out'
    cmd = [sys.executable, str(translator),
           str(base_dir / test['domain']), str(base_dir / test['instance']),
           '--output-file', output_file]
    start = timeit.default_timer()
    proc = subprocess.run(cmd, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, check=False)
    wall_time = timeit.default_timer() - start
    output = proc.stdout.decode('utf-8', errors='replace')
    if os.path.isfile(output_file):
        os.remove(output_file)

    passed = proc.returncode != 0 and test['expected_text'] in output
    status = 'PASSED' if passed else 'FAILED'
    details = ''
    if proc.returncode == 0:
        details += ' [translator unexpectedly succeeded]'
    if test['expected_text'] not in output:
        details += " [missing output: '{}']".format(test['expected_text'])

    print("{} translator rejection test '{}' (time: {:.2f}s){}".format(
        status, test['name'], wall_time, details))

    return TestResult(
        name='rejection-{}'.format(test['name']),
        domain='translator',
        instance_name=test['name'],
        config='rejection',
        passed=passed,
        wall_time=wall_time,
        peak_memory_kb=None,
        expected_cost=None,
        found_cost=None,
        plan_valid=None,
    )


def run_unsolvable_test(test):
    """Run configurations expected to exhaust the search space without a
    plan, checking required output markers (e.g. the registered state
    count)."""
    results = []
    for config in test['configs']:
        cmd = [str(BASEDIR / 'powerlifted.py'),
               '-i', str(BASEDIR / 'dev' / test['instance']),
               '-s', config[0], '-e', config[1], '-g', config[2]]
        start = timeit.default_timer()
        proc = subprocess.run(cmd, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, check=False)
        wall_time = timeit.default_timer() - start
        output = proc.stdout.decode('utf-8', errors='replace')

        missing = [m for m in test['required_output'] if m not in output]
        passed = not missing
        status = 'PASSED' if passed else 'FAILED'
        details = ''
        for marker in missing:
            details += " [missing output: '{}']".format(marker)
        print("{} unsolvable test '{}' with {} (time: {:.2f}s){}".format(
            status, test['name'], ','.join(config), wall_time, details))

        results.append(TestResult(
            name='{}[{}]'.format(test['name'], ','.join(config)),
            domain='unsolvable',
            instance_name=test['name'],
            config=','.join(config),
            passed=passed,
            wall_time=wall_time,
            peak_memory_kb=None,
            expected_cost=None,
            found_cost=None,
            plan_valid=None,
        ))
    return results


def run_unit_test(test):
    start = timeit.default_timer()
    proc = subprocess.run(test['cmd'], stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, check=False)
    wall_time = timeit.default_timer() - start
    passed = proc.returncode == 0
    status = 'PASSED' if passed else 'FAILED'
    print("{} unit test '{}' (time: {:.2f}s)".format(
        status, test['name'], wall_time))
    if not passed:
        print(proc.stdout.decode('utf-8', errors='replace'))

    return TestResult(
        name='unit-{}'.format(test['name']),
        domain='unit',
        instance_name=test['name'],
        config='unit',
        passed=passed,
        wall_time=wall_time,
        peak_memory_kb=None,
        expected_cost=None,
        found_cost=None,
        plan_valid=None,
    )


def run_plan_test_cases(results, test_cases):
    for test_case in test_cases:
        for config in test_case['configs']:
            required_output = list(test_case.get('required_output', []))
            required_output.extend(test_case.get('required_output_by_config', {}).get(config, []))
            test = TestRun(test_case['instance'],
                           config,
                           validate=test_case['validate'],
                           label=test_case['label'],
                           required_output=required_output,
                           extra_args=test_case.get('extra_args_by_config', {}).get(config, []))
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
    heuristic_plan_tests = list(HEURISTIC_PLAN_TESTS)
    novelty_plan_tests = list(NOVELTY_PLAN_TESTS)

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

    run_plan_test_cases(results, SPECIAL_PLAN_TESTS)
    run_plan_test_cases(results, heuristic_plan_tests)
    run_plan_test_cases(results, novelty_plan_tests)
    run_plan_test_cases(results, AXIOM_PLAN_TESTS)

    for unsolvable_test in AXIOM_UNSOLVABLE_TESTS:
        results.extend(run_unsolvable_test(unsolvable_test))

    for rejection_test in TRANSLATOR_REJECTION_TESTS:
        results.append(run_translator_rejection_test(
            rejection_test, BASEDIR / 'dev'))
    if DOWNWARD_BENCHMARKS:
        for rejection_test in IPC_AXIOM_REJECTION_TESTS:
            results.append(run_translator_rejection_test(
                rejection_test, Path(DOWNWARD_BENCHMARKS)))
    else:
        print("Skipping IPC axiom rejection tests "
              "(DOWNWARD_BENCHMARKS not set).")

    for unit_test in UNIT_TESTS:
        results.append(run_unit_test(unit_test))

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
