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

def compute_nodes_per_sec(run):
    if run['coverage'] == 1:
        run['nodes_per_sec'] = float(run['generated'])/max(run['total_time'],0.1)
    return run

def parse_total_time(run):
    if 'search_time' in run and 'total_time' not in run:
        run['total_time'] = run['search_time']
    if 'total_time' not in run or run['total_time'] > 1:
        return True
    return False

def same_domain(run):
    run['problem'] = run['problem'] + '-' + run['domain']
    run['domain'] = 'domain'
    return run

exp.add_report(
    BaseReport(attributes=['generated', 'total_time', 'nodes_per_sec'],
               filter=[parse_total_time,compute_nodes_per_sec],
               filter_algorithm=['blind-full-reducer', 'blind-yannakakis', 'issue311-blind']),
    outfile='nodes-per-sec-report.html')

for alg in ['blind-yannakakis','blind-full-reducer']:
    exp.add_report(
        ScatterPlotReport(
            attributes=['nodes_per_sec'],
            filter_algorithm=['issue311-blind', alg],
            filter=[parse_total_time,compute_nodes_per_sec],
            get_category=domain_as_category,
            format='tex'
        ),
        outfile='{}-{}-vs-{}'.format('nodes_per_sec', 'issue311-blind', alg) + '.tex'
    )

exp.run_steps()
