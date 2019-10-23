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
    if 'planner_time' not in run:
        if 'search_time' in run:
            run['planner_time'] = run['search_time']
    return run

exp = Experiment('data/combine-eval')
exp.add_fetcher('/infai/blaas/thesis/powerlifted/reports/experiments/genome-edit-distance/data/v1-ged-eval')
exp.add_fetcher('/infai/blaas/thesis/powerlifted/reports/experiments/original-org-synth/data/v3-eval')
exp.add_fetcher('/infai/blaas/thesis/powerlifted/reports/experiments/pipesworld-tankage-exp/data/v1-eval')
exp.add_fetcher('/infai/blaas/planners/fast-downward/experiments/thesis/data/thesis-v1-ged-eval')
exp.add_fetcher('/infai/blaas/planners/fast-downward/experiments/thesis/data/thesis-v1-org-synth-eval')
exp.add_fetcher('/infai/blaas/planners/fast-downward/experiments/thesis/data/thesis-v1-pipesworld-eval')
exp.add_report(AbsoluteReport(attributes=['cost', 'coverage', 'generated', 'total_time', 'memory'], filter=[memory_filter, time_filter]))

exp.run_steps()
