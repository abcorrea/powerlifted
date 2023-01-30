import argparse
import os
import sys

def find_domain_filename(task_filename):
    """
    Find domain filename for the given task using automatic naming rules.
    """
    dirname, basename = os.path.split(task_filename)

    domain_basenames = [
        "domain.pddl",
        basename[:3] + "-domain.pddl",
        "domain_" + basename,
        "domain-" + basename,
    ]

    for domain_basename in domain_basenames:
        domain_filename = os.path.join(dirname, domain_basename)
        if os.path.exists(domain_filename):
            return domain_filename


def parse_options():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--domain', dest='domain', action='store',
                                  default=None, help='Domain file in PDDL')
    parser.add_argument('-i', '--instance', dest='instance',
                        action='store', default=None,
                        help='Instance file in PDDL', required=True)
    parser.add_argument('--build', dest='build', action='store_true',
                        help='Build planner before search.')
    parser.add_argument('--debug', dest='debug', action='store_true',
                        help='Run planner in debug mode.')
    parser.add_argument('--cxx-compiler',
                        default='default', help="Path to CXX compiler used by CMake.")
    parser.add_argument('-s', '--search', dest='search', action='store',
                        default='alt-bfws1',
                        help='Search algorithm',
                        choices=("astar",
                                 "bfs",
                                 "bfws1",
                                 "bfws2",
                                 "bfws1-rx",
                                 "bfws2-rx",
                                 "dq-bfws1-rx",
                                 "dq-bfws2-rx",
                                 "alt-bfws1",
                                 "alt-bfws2",
                                 "gbfs",
                                 "iw1",
                                 "iw2",
                                 "lazy",
                                 "lazy-po",
                                 "lazy-prune"))
    parser.add_argument('-e', '--heuristic', dest='heuristic', action='store',
                        default='ff',
                        choices=("blind",
                                 "goalcount",
                                 "add",
                                 "hmax",
                                 "ff",
                                 "rff"),
                        help='Heuristic to guide the search (ignore in case of blind search)')
    parser.add_argument('-g', '--generator', dest='generator', action='store',
                        default='yannakakis', help='Successor generator method',
                        choices=('yannakakis',
                                 'join',
                                 'random_join',
                                 'ordered_join',
                                 'full_reducer'))
    parser.add_argument('--state', action='store', help='Successor generator method',
                        default="sparse", choices=("sparse", "extensional"))
    parser.add_argument('--seed', action='store', help='Random seed.',
                        default=1)
    parser.add_argument('--translator-output-file', dest='translator_file',
                        default='output.lifted',
                        help='Output file of the translator')
    parser.add_argument('--datalog-file', dest='datalog_file',
                        default='model.lp',
                        help='Datalog model for the lifted heuristic.')
    parser.add_argument("--keep-action-predicates", action="store_true",
                        help="flag if the Datalog model should keep action predicates")
    parser.add_argument("--keep-duplicated-rules", action="store_true",
                        help="flag if the Datalog model should keep duplicated auxiliary rules")
    parser.add_argument("--add-inequalities", action="store_true",
                        help="flag if the Datalog model should add inequalities to rules")
    parser.add_argument("--only-effects-novelty-check", action="store_true",
                        help="flag if the novelty evaluation of a state should only consider atoms in the applied action effect")
    parser.add_argument("--novelty-early-stop", action="store_true",
                        help="flag if the novelty evaluation of a state should stop as soon as the w-value is defined")
    parser.add_argument("--unit-cost", action="store_true",
                           help="flag if the actions should be treated as unit-cost actions")
    parser.add_argument("--validate", action="store_true",
                        help="flag if VAL should be called to validate the plan found")
    args = parser.parse_args()
    if args.domain is None:
        args.domain = find_domain_filename(args.instance)
        if args.domain is None:
            raise RuntimeError(f'Could not find domain filename matching instance file "{args.instance}"')

    return args
