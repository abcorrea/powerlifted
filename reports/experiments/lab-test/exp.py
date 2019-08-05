#! /usr/bin/env python

import os
import platform

from suites import OPTIMAL_SUITE, EXCLUDED_DOMAINS

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.experiment import Experiment

from downward import suites
from downward.reports.absolute import AbsoluteReport

from common_setup import Configuration

# Create custom report class with suitable info and error attributes.
class BaseReport(AbsoluteReport):
    INFO_ATTRIBUTES = []
    ERROR_ATTRIBUTES = [
        'domain', 'problem', 'algorithm', 'unexplained_errors', 'error', 'node']

NODE = platform.node()
REMOTE = NODE.endswith(".scicore.unibas.ch") or NODE.endswith(".cluster.bc2.ch")
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
POWER_LIFTED_DIR = os.environ["POWER_LIFTED_SRC"]

if REMOTE:
    SUITE = OPTIMAL_SUITE
    ENV = BaselSlurmEnvironment(
        partition='infai_2',
        export=["PATH", "DOWNWARD_BENCHMARKS"])
else:
    SUITE = ['gripper:prob01.pddl',
             'miconic:s1-0.pddl']
    ENV = LocalEnvironment(processes=4)

TIME_LIMIT = 1800
MEMORY_LIMIT = 16384

ATTRIBUTES=['initial_state_size',
            'peak_memory',
            'cost',
            'coverage',
            'search_time']

# Create a new experiment.
exp = Experiment(environment=ENV)
# Add custom parser for Power Lifted.
exp.add_parser('power-lifted-parser.py')

CONFIGS = [Configuration('blind-full_reducer', ['naive', 'blind', 'full_reducer']),
           Configuration('blind-ordered_join', ['naive', 'blind', 'ordered_join']),
           Configuration('blind-join', ['naive', 'blind', 'join'])]

# Create one run for each instance and each configuration
for config in CONFIGS:
    for task in suites.build_suite(BENCHMARKS_DIR, SUITE):
        if task.domain in EXCLUDED_DOMAINS:
            continue
        run = exp.add_run()
        run.add_resource('domain', task.domain_file, symlink=True)
        run.add_resource('problem', task.problem_file, symlink=True)
        run.add_command(
            'run-translator',
            [POWER_LIFTED_DIR+'/builds/release/translator/translate.py',
             task.domain_file, task.problem_file],
            time_limit=TIME_LIMIT,
            memory_limit=MEMORY_LIMIT)
        run.add_command(
            'run-search',
            [POWER_LIFTED_DIR+'/builds/release/search/search', 'output.lifted'] +
            config.arguments,
            time_limit=TIME_LIMIT,
            memory_limit=MEMORY_LIMIT)
        run.set_property('domain', task.domain)
        run.set_property('problem', task.problem)
        run.set_property('algorithm', config.name)
        run.set_property('id', [config.name, task.domain, task.problem])

        # Add step that writes experiment files to disk.
exp.add_step('build', exp.build)

# Add step that executes all runs.
exp.add_step('start', exp.start_runs)

# Add step that collects properties from run directories and
# writes them to *-eval/properties.
exp.add_fetcher(name='fetch')

# Make a report.
exp.add_report(
    BaseReport(attributes=ATTRIBUTES),
    outfile='report.html')

# Parse the commandline and run the specified steps.
exp.run_steps()
