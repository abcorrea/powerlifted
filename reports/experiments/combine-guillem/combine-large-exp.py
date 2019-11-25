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

def expansions_filter(run):
    if 'visited' in run:
        run['expansions'] = run['visited']
    return run

def memory_filter(run):
    if 'peak_memory' not in run and run['coverage'] == 1:
        run['peak_memory'] = max(run['memory'], run['translator_peak_memory'], run['planner_memory'])
    return run

def time_filter(run):
    if 'search_time' in run:
        run['total_time'] = run['search_time']
    return run

def translate_time_filter(run):
    if 'translator_time_done' not in run and run['coverage'] == 1:
        run['translator_time_done'] = 0.1
    return run

def parse_total_time_pruning(run):
    if 'search_time' in run and 'total_time' not in run:
        run['total_time'] = run['search_time']
    if 'total_time' not in run or run['total_time'] > 1:
        return True
    return False

exp = Experiment('data/combine-eval')
exp.add_fetcher('/home/blaas/thesis/powerlifted/reports/experiments/guillems-planner-exp/data/v1-eval')
exp.add_fetcher('/home/blaas/thesis/powerlifted/reports/experiments/yannakakis/data/v2-eval')

exp.add_report(AbsoluteReport(
    attributes=['cost', 'coverage', 'expansions', 'total_time', 'peak_memory'],
    filter_algorithm=['blind-full-reducer', 'fs-guillem'],
    filter=[expansions_filter, normalize_coverage, time_filter]))

# exp.add_report(
#     BaseReport(attributes=['coverage', 'memory', 'generated', 'planner_time', 'translator_time_done'],
#                filter=[memory_filter,time_filter,translate_time_filter]),
#     outfile='report.html')

for attr in ['expansions', 'total_time', 'peak_memory']:
     for alg in ['blind-full-reducer']:
         exp.add_report(
             ScatterPlotReport(
                 attributes=[attr],
                 filter_algorithm=['fs-guillem', alg],
                 filter=[expansions_filter, normalize_coverage, time_filter],
                 get_category=domain_as_category,
                 format='tex'
             ),
             outfile='{}-{}-vs-{}'.format(attr, alg, 'fs-guillem') + '.tex'
         )



exp.run_steps()
