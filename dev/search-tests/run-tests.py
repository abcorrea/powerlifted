#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import subprocess
import sys

from distutils.dir_util import copy_tree
from itertools import product
from shutil import copyfile

"""
This test component just checks to see if the plans found
in several different domains and configurations have the optimal
cost expected.

The expected run time of each run is less than 30s.

"""


# Commented instances have costs which are currently ignored
OPTIMAL_PLAN_COSTS = {'airport-p01.lifted': 8,
                      'airport-p05.lifted': 21,
                      'blocks-4-0.lifted': 6,
                      'gripper-prob01.lifted': 11,
                      'movie-prob30.lifted':  7,
                      #'openstacks-opt08-strips-p01.lifted': 2,
                      'organic-synthesis-opt18-strips-p05.lifted': 2}

SEARCH_CONFIGS = ['naive', 'gbfs']
HEURISTIC_CONFIGS = ['blind']
GENERATOR_CONFIGS = ['full_reducer', 'join', 'ordered_join', 'yannakakis']
STATE_REPR_CONFIGS = ['sparse', 'extensional']


class TestRun:
    def __init__(self, instance, config, build):
        self.instance = instance
        self.search = config[0]
        self.heuristic = config[1]
        self.generator = config[2]
        self.state_representation = config[3]
        self.build = build

    def get_config(self):
        return "{}, {}, {}, and {}".format(self.search,
                                           self.heuristic,
                                           self.generator,
                                           self.state_representation)

    def __str__(self):
        return ("{} with {}".format(self.instance, self.get_config()))

    def run(self):
        print ("Testing {} with {}...".format(self.instance, self.get_config()), end='', flush=True)
        output = subprocess.check_output([os.path.join(self.build, 'search', 'search'),
                                          '-f', self.instance,
                                          '-s', self.search,
                                          '-e', self.heuristic,
                                          '-g', self.generator,
                                          '-r', self.state_representation])
        return output

    def evaluate(self, output, optimal_cost):
        cost_plan_found = None
        for line in output.splitlines():
            if b'Total plan cost:' in line:
                cost_plan_found = int(line.split()[3])
                break

        if cost_plan_found == optimal_cost:
            return True
        else:
            return False

    def remove_plan_file(self):
        plan_file = 'sas_plan'
        if os.path.isfile(plan_file):
            os.remove(plan_file)



if __name__ == '__main__':

    if len(sys.argv) != 2:
        print("Usage: ./run-tests.py BUILD_DIRECTORY")
        sys.exit()


    BUILD_DIR = os.path.join(sys.argv[1])
    if not os.path.exists(BUILD_DIR):
        raise FileNotFoundError("Build directory %s does not exist." % BUILD_DIR)


    for instance, cost in OPTIMAL_PLAN_COSTS.items():
        for config in product(SEARCH_CONFIGS, HEURISTIC_CONFIGS, GENERATOR_CONFIGS, STATE_REPR_CONFIGS):
            test = TestRun(instance, config, BUILD_DIR)
            output = test.run()
            passed = test.evaluate(output, cost)
            if passed:
                print(" PASS")
            else:
                print(" FAIL")
            test.remove_plan_file()
