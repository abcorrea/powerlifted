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
exp.add_fetcher('data/v1-eval')
exp.add_fetcher('data/v2-eval')

exp.add_report(
    BaseReport(attributes=['largest_relation'],
               format='html'),
    outfile='report.html')

for attr in ['largest_relation']:
    for alg in ["blind-ordered_join", "blind-join"]:
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


exp.run_steps()
