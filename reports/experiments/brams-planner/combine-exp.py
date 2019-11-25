#! /usr/bin/env python

import os
import platform

from suites import OPTIMAL_SUITE, EXCLUDED_DOMAINS, CYCLIC_SCHEMAS

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.experiment import Experiment

from downward import suites
from downward.reports.absolute import AbsoluteReport
from downward.reports.scatter import ScatterPlotReport

from common_setup import Configuration
from exp_utils import *

class BaseReport(AbsoluteReport):
    INFO_ATTRIBUTES = []
    ERROR_ATTRIBUTES = [
        'domain', 'problem', 'algorithm', 'unexplained_errors', 'error', 'node']

def normalize_coverage(run):
    if 'coverage' in run:
        run['coverage'] = min(1, run['coverage'])
    else:
        run['coverage'] = 0
    return run

def normalize_time(run):
    if run['coverage'] == 0:
        run['search_time'] = 10000
    return run

exp = Experiment('data/combine-eval')
exp.add_fetcher('/home/blaas/thesis/powerlifted/reports/experiments/brams-planner/data/v1-hard-domains-eval')
exp.add_fetcher('/home/blaas/thesis/powerlifted/reports/experiments/naive-join-htg/data/v1-eval')


exp.add_report(AbsoluteReport(
    attributes=['coverage', 'search_time', 'peak_memory'],
    filter_algorithm=['goalcount-yannakakis', 'lrpg'],
    filter=[normalize_coverage]))

for attr in ['search_time']:
     for alg in ['goalcount-yannakakis']:
         exp.add_report(
             ScatterPlotReport(
                 attributes=[attr],
                 filter_algorithm=['lrpg', alg],
                 filter=[normalize_coverage, normalize_time],
                 get_category=domain_as_category,
                 format='tex'
             ),
             outfile='{}-{}-vs-{}'.format(attr, alg, 'lrpg') + '.tex'
         )



exp.run_steps()
