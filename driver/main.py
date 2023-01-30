import errno
import logging
import os
import subprocess
import sys
from pathlib import Path

from build import build, PROJECT_ROOT

from . import arguments


def validate(domain_name, instance_name, planfile):
    plan = Path(planfile)
    if not plan.is_file():
        logging.info(f'No plan file to validate could be found at "{planfile}"')
        return

    try:
        validate_inputs = ["validate", domain_name, instance_name, planfile]
        _ = subprocess.call(' '.join(validate_inputs), shell=True)
    except OSError as err:
        if err.errno == errno.ENOENT:
            logging.error("Error: 'validate' binary not found. Is it on the PATH?")
            return
        else:
            logging.error(f"Error executing 'validate': {err}")


def main():
    CPP_EXTRA_OPTIONS = []
    PYTHON_EXTRA_OPTIONS = []

    options = arguments.parse_options()

    build_dir = os.path.join(PROJECT_ROOT, 'builds', 'debug' if options.debug else 'release')

    if options.build:
        build(options.debug, options.cxx_compiler)

    # Create build path
    if not os.path.exists(build_dir):
        raise OSError("Planner not built!")

    # If it is the lifted heuristic, we need to obtain the Datalog model
    if options.heuristic in ['add', 'hmax'] or options.search in ['bfws1-rx', 'bfws2-rx', 'dq-bfws1-rx', 'dq-bfws2-rx', 'alt1', 'alt2']:
       PYTHON_EXTRA_OPTIONS += ['--build-datalog-model', '--datalog-file', options.datalog_file]
       if options.keep_action_predicates:
           PYTHON_EXTRA_OPTIONS.append('--keep-action-predicates')
       if options.keep_duplicated_rules:
           PYTHON_EXTRA_OPTIONS.append('--keep-duplicated-rules')
       if options.add_inequalities:
           PYTHON_EXTRA_OPTIONS.append('--add-inequalities')
       CPP_EXTRA_OPTIONS += ['--datalog-file', options.datalog_file]


    # If it is a width-based search, we might need to pass more flags
    if options.only_effects_novelty_check:
        CPP_EXTRA_OPTIONS += ['--only-effects-novelty-check', str(1)]
    if options.novelty_early_stop:
        CPP_EXTRA_OPTIONS += ['--novelty-early-stop', str(1)]


    # Checks if unit-cost flag is true
    if options.unit_cost:
        PYTHON_EXTRA_OPTIONS += ["--unit-cost"]


    # Invoke the Python preprocessor
    translator = subprocess.Popen([os.path.join(build_dir, 'translator', 'translate.py'),
                                   options.domain, options.instance, '--output-file', options.translator_file] + PYTHON_EXTRA_OPTIONS)
    translator.communicate()
    if translator.returncode != 0:
        raise RuntimeError("Error during preprocessing/translation.")



    # Invoke the C++ search component
    cmd = [os.path.join(build_dir, 'search', 'search'),
           '-f', options.translator_file,
           '-s', options.search,
           '-e', options.heuristic,
           '-g', options.generator,
           '-r', options.state,
           '--seed', str(options.seed)] + \
           CPP_EXTRA_OPTIONS

    print(f'Executing "{" ".join(cmd)}"')
    code = subprocess.call(cmd)

    # If we found a plan, try to validate it
    if code == 0 and options.validate:
        validate(options.domain, options.instance, 'sas_plan')

    return code
