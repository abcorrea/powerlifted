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
    return run

def time_filter(run):
    if 'total_time' in run and 'planner_time' in run:
        run['total_time'] = run['planner_time']
    if 'search_time' in run and 'total_time' not in run:
        run['total_time'] = run['search_time']
    return run

exp = Experiment('data/combine-blind-eval')

def compute_nodes_per_sec(run):
    if run['coverage'] == 1:
        run['nodes_per_sec'] = float(run['generated'])/max(run['total_time'],0.1)
    return run

def parse_total_time_pruning(run):
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
               filter=[parse_total_time_pruning,compute_nodes_per_sec],
               filter_algorithm=['blind-full-reducer', 'blind-yannakakis', 'issue311-blind']),
    outfile='nodes-per-sec-report.html')

for alg in ['blind-yannakakis','blind-full-reducer']:
    exp.add_report(
        ScatterPlotReport(
            attributes=['nodes_per_sec'],
            filter_algorithm=['issue311-blind', alg],
            filter=[parse_total_time_pruning,compute_nodes_per_sec,discriminate_org_synt],
            get_category=domain_as_category,
            format='tex'
        ),
        outfile='{}-{}-vs-{}'.format('nodes_per_sec', 'issue311-blind', alg) + '.tex'
    )

exp.add_report(
    ScatterPlotReport(
        attributes=['nodes_per_sec'],
        filter_algorithm=['blind-yannakakis','blind-full-reducer'],
        filter=[parse_total_time_pruning,compute_nodes_per_sec,discriminate_org_synt],
        get_category=domain_as_category,
        format='tex'
    ),
    outfile='{}-{}-vs-{}'.format('nodes_per_sec', 'blind-yannakakis','blind-full-reducer') + '.tex'
)

    
for attr in ['total_time', 'memory']:
    for alg in ['blind-yannakakis','blind-full-reducer']:
        exp.add_report(
            ScatterPlotReport(
                attributes=[attr],
                filter_algorithm=['issue311-blind', alg],
                filter=[time_filter, memory_filter,discriminate_org_synt],
                get_category=domain_as_category,
                format='tex'
            ),
            outfile='{}-{}-vs-{}'.format(attr, alg, 'issue311-blind') + '.tex'
        )

for attr in ['total_time', 'memory']:
    for alg in ['goalcount-yannakakis','goalcount-full-reducer']:
        exp.add_report(
            ScatterPlotReport(
                attributes=[attr],
                filter_algorithm=['issue311-goalcount', alg],
                filter=[time_filter, memory_filter,discriminate_org_synt],
                get_category=domain_as_category,
                format='tex'
            ),
            outfile='{}-{}-vs-{}'.format(attr, alg, 'issue311-goalcount') + '.tex'
        )

exp.run_steps()
