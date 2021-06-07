# -*- coding: utf-8 -*-

import argparse
import sys


def parse_args():
    argparser = argparse.ArgumentParser()
    argparser.add_argument(
        "domain", help="path to domain pddl file")
    argparser.add_argument(
        "task", help="path to task pddl file")
    argparser.add_argument(
        "--output-file", default="output.lifted",
        help="path to the output file (default: %(default)s)")
    argparser.add_argument(
        "--ground-state-representation", action="store_true",
        help="use a complete ground state representation, where each possible "
             "ground atom corresponds to a bit of the state, instead of a "
             "sparse representation.")
    argparser.add_argument(
        "--build-datalog-model", action="store_true",
        help="flag if the translator should output Datalog model of the task.")
    argparser.add_argument(
        "--datalog-file", default='model.lp',
        help="flag if the translator should output Datalog model of the task.")
    argparser.add_argument(
        "--keep-action-predicates", action="store_true",
        help="flag if the Datalog model should keep action predicates")
    argparser.add_argument(
        "--keep-duplicated-rules", action="store_true",
        help="flag if the Datalog model should keep duplicated auxiliary rules")
    argparser.add_argument(
        "--add-inequalities", action="store_true",
        help="flag if the Datalog model should add inequalities to rules")
    argparser.add_argument(
        "--unit-cost", action="store_true",
        help="flag if the actions should be treated as unit-cost actions")
    argparser.add_argument(
        "--test-experiment", action="store_true",
        help="flag if the run is an experiment or not")
    argparser.add_argument(
        "--verbose-data", action="store_true",
        help="flag if the translator should output more statistical data than "
             "normal, in a format that is easier to parse for our evaluation "
             "scripts.")
    return argparser.parse_args()


def copy_args_to_module(args):
    module_dict = sys.modules[__name__].__dict__
    for key, value in vars(args).items():
        module_dict[key] = value


def setup():
    args = parse_args()
    copy_args_to_module(args)


setup()
