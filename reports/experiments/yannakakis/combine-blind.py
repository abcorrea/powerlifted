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


def memory_filter(run):
    if 'peak_memory' in run:
        run['memory'] = run['peak_memory']
    else:
        run['memory'] = None
    return run

def time_filter(run):
    if 'total_time' not in run:
        if 'search_time' in run:
            run['total_time'] = run['search_time']
    return run

exp = Experiment('data/combined-blind-v1')
exp.add_fetcher('data/v2-eval')
exp.add_fetcher('/infai/blaas/issues/issue311/experiments/thesis/data/thesis-v1-eval')
exp.add_fetcher('data/goalcount-v1-eval')
exp.add_report(AbsoluteReport(attributes=['cost', 'coverage', 'generated', 'total_time', 'memory'], filter=[memory_filter, time_filter]))

exp.add_report(AbsoluteReport(attributes=['coverage'],
                              filter=[memory_filter, time_filter],
                              filter_algorithm=['blind-full-reducer', 'issue311-blind'],),
               outfile='coverage-blind.html')

exp.add_report(AbsoluteReport(attributes=['coverage'],
                              filter=[memory_filter, time_filter],
                              filter_algorithm=['goalcount-full-reducer', 'issue311-goalcount'],),
               outfile='coverage-goalcount.html')

exp.run_steps()
