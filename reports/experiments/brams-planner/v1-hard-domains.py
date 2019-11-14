#! /usr/bin/env python


# BEFORE RUNNING:
# - Copy planner.img to the experiment directory

import os
import platform

from suites import OPTIMAL_SUITE, EXCLUDED_DOMAINS

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.experiment import Experiment

from downward import suites
from downward.reports.absolute import AbsoluteReport
from downward.reports.scatter import ScatterPlotReport

from common_setup import Configuration
from exp_utils import *

"""
Test memory and time to instantiate initial state.
"""

# Create custom report class with suitable info and error attributes.
class BaseReport(AbsoluteReport):
    INFO_ATTRIBUTES = []
    ERROR_ATTRIBUTES = [
        'domain', 'problem', 'algorithm', 'unexplained_errors', 'error', 'node']

NODE = platform.node()
REMOTE = NODE.endswith(".scicore.unibas.ch") or NODE.endswith(".cluster.bc2.ch")
POWER_LIFTED_DIR = os.environ["POWER_LIFTED_SRC"]
DOWNWARD_BENCHMARKS = os.environ["DOWNWARD_BENCHMARKS"]
BENCHMARKS_DIR = POWER_LIFTED_DIR+'/local-tests'

if REMOTE:
    SUITE = ['genome-edit-distance',
             'genome-edit-distance-split',
             'organic-synthesis-alkene',
             'organic-synthesis-MIT',
             'organic-synthesis-original',
             'pipesworld-tankage-nosplit']
    ENV = BaselSlurmEnvironment(
        partition='infai_2',
        memory_per_cpu="6G",
        extra_options='#SBATCH --cpus-per-task=3',
        export=["PATH", "DOWNWARD_BENCHMARKS", "POWER_LIFTED_DIR"])
else:
    SUITE = ['gripper:prob01.pddl',
             'miconic:s1-0.pddl']
    ENV = LocalEnvironment(processes=4)

TIME_LIMIT = 1800
MEMORY_LIMIT = 16384

ATTRIBUTES=['coverage',
            'visited',
            'peak_memory',
            'search_time']


# Create a new experiment.
exp = Experiment(environment=ENV)

# Add custom parser for Power Lifted.
exp.add_parser('parser.py')

# Create one run for each instance and each configuration
task_id = 1
lower = 1
upper = 100
results_dir=os.path.dirname(os.path.realpath(__file__))+'/data/v1-hard-domains/'
for task in suites.build_suite(BENCHMARKS_DIR, SUITE):
    run = exp.add_run()
    run_batch = 'runs-{:0>5}-{:0>5}/'.format(lower, upper)
    run_number = '{:0>5}'.format(task_id)
    run_dir = os.path.join(results_dir, run_batch, run_number)
    run.add_resource('domain', task.domain_file)
    run.add_resource('problem', task.problem_file)
    #run.add_resource('planner',  'planner.img')
    run.add_command( 'run-search', ['singularity', 'run', '-C', '-H',
                                    run_dir, '/scratch/singularity/lrpg.img',
                                    os.path.basename(task.domain_file),
                                    os.path.basename(task.problem_file)],
                     time_limit=TIME_LIMIT,
                     memory_limit=MEMORY_LIMIT)
    run.set_property('domain', task.domain)
    run.set_property('problem', task.problem)
    run.set_property('algorithm', 'lrpg')
    run.set_property('id', ['lrpg', task.domain, task.problem])
    task_id += 1
    if task_id > upper:
        lower += 100
        upper += 100


exp.add_step('build', exp.build)

# Add step that executes all runs.
exp.add_step('start', exp.start_runs)

# Add step that collects properties from run directories and
# writes them to *-eval/properties.
exp.add_fetcher(name='fetch')

def normalize_coverage(run):
    if 'coverage' in run:
        run['coverage'] = 1
    else:
        run['coverage'] = 0
    return run

# Make a report.
exp.add_report(
    BaseReport(attributes=ATTRIBUTES,
    filter=[normalize_coverage]),
    outfile='report.html')

# Parse the commandline and run the specified steps.
exp.run_steps()
