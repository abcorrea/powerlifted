#! /usr/bin/env python

import os
import platform

from suites import ORG_SYNTHESIS_MIT, EXCLUDED_DOMAINS

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
BENCHMARKS_DIR = POWER_LIFTED_DIR+"local-tests/"

if REMOTE:
    SUITE = ["pipesworld-tankage-nosplit:p01-net1-b6-g2-t50.pddl",
             "pipesworld-tankage-nosplit:p02-net1-b6-g4-t50.pddl",
             "pipesworld-tankage-nosplit:p03-net1-b8-g3-t80.pddl",
             "pipesworld-tankage-nosplit:p04-net1-b8-g5-t80.pddl",
             "pipesworld-tankage-nosplit:p05-net1-b10-g4-t50.pddl",
             "pipesworld-tankage-nosplit:p06-net1-b10-g6-t50.pddl",
             "pipesworld-tankage-nosplit:p07-net1-b12-g5-t80.pddl",
             "pipesworld-tankage-nosplit:p08-net1-b12-g7-t80.pddl",
             "pipesworld-tankage-nosplit:p09-net1-b14-g6-t50.pddl",
             "pipesworld-tankage-nosplit:p10-net1-b14-g8-t50.pddl",
             "pipesworld-tankage-nosplit:p11-net2-b10-g2-t30.pddl",
             "pipesworld-tankage-nosplit:p12-net2-b10-g4-t60.pddl",
             "pipesworld-tankage-nosplit:p13-net2-b12-g3-t70.pddl",
             "pipesworld-tankage-nosplit:p14-net2-b12-g5-t30.pddl",
             "pipesworld-tankage-nosplit:p15-net2-b14-g4-t30.pddl",
             "pipesworld-tankage-nosplit:p16-net2-b14-g6-t80.pddl",
             "pipesworld-tankage-nosplit:p17-net2-b16-g5-t20.pddl",
             "pipesworld-tankage-nosplit:p18-net2-b16-g7-t60.pddl",
             "pipesworld-tankage-nosplit:p19-net2-b18-g6-t60.pddl",
             "pipesworld-tankage-nosplit:p20-net2-b18-g8-t90.pddl",
             "pipesworld-tankage-nosplit:p21-net3-b12-g2-t60.pddl",
             "pipesworld-tankage-nosplit:p22-net3-b12-g4-t60.pddl",
             "pipesworld-tankage-nosplit:p23-net3-b14-g3-t60.pddl",
             "pipesworld-tankage-nosplit:p24-net3-b14-g5-t60.pddl",
             "pipesworld-tankage-nosplit:p25-net3-b16-g5-t60.pddl",
             "pipesworld-tankage-nosplit:p26-net3-b16-g7-t70.pddl",
             "pipesworld-tankage-nosplit:p27-net3-b18-g6-t70.pddl",
             "pipesworld-tankage-nosplit:p28-net3-b18-g7-t70.pddl",
             "pipesworld-tankage-nosplit:p29-net3-b20-g6-t70.pddl",
             "pipesworld-tankage-nosplit:p30-net3-b20-g8-t70.pddl",
             "pipesworld-tankage-nosplit:p31-net4-b14-g3-t20.pddl",
             "pipesworld-tankage-nosplit:p32-net4-b14-g5-t30.pddl",
             "pipesworld-tankage-nosplit:p33-net4-b16-g5-t60.pddl",
             "pipesworld-tankage-nosplit:p34-net4-b16-g6-t60.pddl",
             "pipesworld-tankage-nosplit:p35-net4-b18-g4-t90.pddl",
             "pipesworld-tankage-nosplit:p36-net4-b18-g6-t90.pddl",
             "pipesworld-tankage-nosplit:p37-net4-b20-g5-t60.pddl",
             "pipesworld-tankage-nosplit:p38-net4-b20-g7-t60.pddl",
             "pipesworld-tankage-nosplit:p39-net4-b22-g7-t50.pddl",
             "pipesworld-tankage-nosplit:p40-net4-b22-g8-t50.pddl",
             "pipesworld-tankage-nosplit:p41-net5-b22-g2-t20.pddl",
             "pipesworld-tankage-nosplit:p42-net5-b22-g4-t50.pddl",
             "pipesworld-tankage-nosplit:p43-net5-b24-g3-t80.pddl",
             "pipesworld-tankage-nosplit:p44-net5-b24-g5-t80.pddl",
             "pipesworld-tankage-nosplit:p45-net5-b26-g4-t50.pddl",
             "pipesworld-tankage-nosplit:p46-net5-b26-g6-t50.pddl",
             "pipesworld-tankage-nosplit:p47-net5-b28-g5-t50.pddl",
             "pipesworld-tankage-nosplit:p48-net5-b28-g7-t50.pddl",
             "pipesworld-tankage-nosplit:p49-net5-b30-g6-t50.pddl",
             "pipesworld-tankage-nosplit:p50-net5-b30-g8-t50.pddl",]
    ENV = BaselSlurmEnvironment(
        partition='infai_2',
        memory_per_cpu="6G",
        extra_options='#SBATCH --cpus-per-task=3',
        export=["PATH", "DOWNWARD_BENCHMARKS", "POWER_LIFTED_DIR"])
else:
    SUITE = ['blocks-large:p76.pddl']
    ENV = LocalEnvironment(processes=4)

TIME_LIMIT = 1800
MEMORY_LIMIT = 16384

ATTRIBUTES=['cost',
            'coverage',
            'generated',
            'initial_state_size',
            'peak_memory',
            'search_time',
            'visited']

# Create a new experiment.
exp = Experiment(environment=ENV)

# Add custom parser for Power Lifted.
exp.add_parser('power-lifted-parser.py')

CONFIGS = [Configuration('blind-full-reducer', ['naive', 'blind', 'full_reducer']),
           Configuration('blind-yannakakis', ['naive', 'blind', 'yannakakis']),
           Configuration('goalcount-full-reducer', ['gbfs', 'goalcount', 'full_reducer']),
           Configuration('goalcount-yannakakis', ['gbfs', 'goalcount', 'yannakakis'])]

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

exp.add_report(
    BaseReport(attributes=ATTRIBUTES,
               format='tex'),
    outfile='report.tex')

for attr in ['search_time', 'generated']:
    for alg in ["blind-join", "blind-ordered_join", "blind-yannakakis"]:
        exp.add_report(
            ScatterPlotReport(
                attributes=[attr],
                filter_algorithm=[alg, 'blind-full-reducer'],
                filter=[discriminate_acyclic],
                get_category=domain_as_category,
                format='tex'
            ),
            outfile='{}-{}-vs-{}'.format(attr, alg, "blind-full-reducer") + '.tex'
        )

# Parse the commandline and run the specified steps.
exp.run_steps()
