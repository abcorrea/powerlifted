#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import os
import subprocess
import timeit

from itertools import product

"""
This test component just checks to see if the plans found
in several different domains and configurations have the optimal
cost expected.

The expected run time of each run is less than 30s.

"""

BASEDIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))


# Commented instances have costs which are currently ignored
OPTIMAL_PLAN_COSTS = {'domains/airport/p05-airport2-p1.pddl': 21,
                      'domains/blocks/probBLOCKS-4-0.pddl': 6,
                      'domains/gripper/prob01.pddl': 11,
                      'domains/movie/prob30.pddl': 7,
                      'domains/openstacks/p01.pddl': 2,
                      'domains/organic-synthesis/p05.pddl': 2}
SEARCH_CONFIGS = ['bfs', 'gbfs']
HEURISTIC_CONFIGS = ['blind']
GENERATOR_CONFIGS = ['full_reducer', 'join', 'yannakakis']
STATE_REPR_CONFIGS = ['sparse', 'extensional']


class TestRun:
    def __init__(self, instance, config):
        self.instance = instance
        self.search = config[0]
        self.heuristic = config[1]
        self.generator = config[2]
        self.state_representation = config[3]

    def get_config(self):
        return "{}, {}, {}, and {}".format(self.search,
                                           self.heuristic,
                                           self.generator,
                                           self.state_representation)

    def __str__(self):
        return "{} with {}".format(self.instance, self.get_config())

    def run(self):
        print("Testing {} with {}: ".format(self.instance, self.get_config()), end='', flush=True)
        output = subprocess.check_output([os.path.join(BASEDIR, 'powerlifted.py'),
                                          '-i', os.path.join(BASEDIR, 'dev', self.instance),
                                          '-s', self.search,
                                          '-e', self.heuristic,
                                          '-g', self.generator,
                                          '--state', self.state_representation,
                                          '--validate'])
        return output

    def evaluate(self, output, optimal_cost):
        plan_length_found = None
        plan_valid = None
        for line in output.splitlines():
            if b'Total plan cost:' in line:
                plan_length_found = int(line.split()[3])
            if b'Plan valid' in line:
                plan_valid = True

        if plan_length_found == optimal_cost and plan_valid:
            print("PASSED")
            return True
        else:
            print("FAILED ", end="")
            if plan_length_found != optimal_cost:
                print("[expected: {}, plan length found: {}]".format(optimal_cost, plan_length_found), end="")
            if not plan_valid:
                print("[VAL did not validate the plan]", end="")
            print("")
            return False

    def remove_plan_file(self):
        plan_file = 'sas_plan'
        if os.path.isfile(plan_file):
            os.remove(plan_file)


def print_summary(passes, failures, starting_time):
    total = passes + failures
    print("Total number of passed tests: %d/%d" % (passes, total))
    print("Total number of failed tests: %d/%d" % (failures, total))
    print("Total time: %.2fs" % (timeit.default_timer() - starting_time))


def parse_options():
    parser = argparse.ArgumentParser()
    parser.add_argument('--minimal', dest='minimal', action='store_true',
                        help='Use minimal test set.')
    args = parser.parse_args()

    return args


if __name__ == '__main__':
    args = parse_options()

    if args.minimal:
        OPTIMAL_PLAN_COSTS = {'domains/blocks/probBLOCKS-4-0.pddl': 6,
                              'domains/gripper/prob01.pddl': 11,
                              'domains/movie/prob30.pddl': 7}
        SEARCH_CONFIGS = ['bfs', 'gbfs']
        HEURISTIC_CONFIGS = ['blind']
        GENERATOR_CONFIGS = ['full_reducer', 'yannakakis']
        STATE_REPR_CONFIGS = ['sparse', 'extensional']

    start = timeit.default_timer()
    failures = 0
    passes = 0
    for instance, cost in OPTIMAL_PLAN_COSTS.items():
        for config in product(SEARCH_CONFIGS, HEURISTIC_CONFIGS, GENERATOR_CONFIGS, STATE_REPR_CONFIGS):
            test = TestRun(instance, config)
            output = test.run()
            passed = test.evaluate(output, cost)
            if passed:
                passes += 1
            else:
                failures += 1
            test.remove_plan_file()

    print_summary(passes, failures, start)
