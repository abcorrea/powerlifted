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


def run_single_search(build_dir, translator_file, search, evaluator, generator, state, seed, plan_file, extra):
        cmd = [os.path.join(build_dir, 'search', 'search'),
               '-f', translator_file,
               '-s', search,
               '-e', evaluator,
               '-g', generator,
               '-r', state,
               '--seed', seed] + \
                   ['--plan-file', plan_file] +\
               extra
        print(f'Executing "{" ".join(cmd)}"')
        code = subprocess.call(cmd)
        return code

def run_search(build_dir, options, extra):
    if options.iteration is None:
        code = run_single_search(build_dir,
                                 options.translator_file,
                                 options.search,
                                 options.heuristic,
                                 options.generator,
                                 options.state,
                                 str(options.seed),
                                 'plan',
                                 extra)

        # If we found a plan, try to validate it
        if code == 0 and options.validate:
            validate(options.domain, options.instance, 'plan')
        return code
    else:
        has_found_plan = False
        for count, it in enumerate(options.iteration):
            search, evaluator, generator = it.split(',')
            plan_name = 'plan.'+str(count+1)
            code = run_single_search(build_dir,
                                     options.translator_file,
                                     search,
                                     evaluator,
                                     generator,
                                     options.state,
                                     str(options.seed),
                                     plan_name,
                                     extra)

            # If we found a plan, check if we need to validate it
            # and then quit iterations
            if code == 0:
                has_found_a_plan = True
                if options.validate:
                    validate(options.domain, options.instance, plan_name)
        if has_found_plan:
            return 0
        else:
            return -1

def run_translator(build_dir, options, extra):
    translator = subprocess.Popen([os.path.join(build_dir, 'translator', 'translate.py'),
                                   options.domain, options.instance, '--output-file', options.translator_file] + extra)
    translator.communicate()
    if translator.returncode != 0:
        raise RuntimeError("Error during preprocessing/translation.")



def set_extra_options(options):
    CPP_EXTRA_OPTIONS = []
    PYTHON_EXTRA_OPTIONS = []

    if options.keep_action_predicates:
        PYTHON_EXTRA_OPTIONS.append('--keep-action-predicates')
    if options.keep_duplicated_rules:
        PYTHON_EXTRA_OPTIONS.append('--keep-duplicated-rules')
    if options.add_inequalities:
        PYTHON_EXTRA_OPTIONS.append('--add-inequalities')


    # If it is a width-based search, we might need to pass more flags
    if options.only_effects_novelty_check:
        CPP_EXTRA_OPTIONS += ['--only-effects-novelty-check', str(1)]
    if options.novelty_early_stop:
        CPP_EXTRA_OPTIONS += ['--novelty-early-stop', str(1)]


    # Checks if unit-cost flag is true
    if options.unit_cost:
        PYTHON_EXTRA_OPTIONS += ["--unit-cost"]

    return PYTHON_EXTRA_OPTIONS, CPP_EXTRA_OPTIONS




def main():

    options = arguments.parse_options()

    build_dir = os.path.join(PROJECT_ROOT, 'builds', 'debug' if options.debug else 'release')

    if options.build:
        build(options.debug, options.cxx_compiler)

    # Create build path
    if not os.path.exists(build_dir):
        raise OSError("Planner not built!")


    PYTHON_EXTRA_OPTIONS, CPP_EXTRA_OPTIONS = set_extra_options(options)

    # Invoke the Python preprocessor
    run_translator(build_dir, options, PYTHON_EXTRA_OPTIONS)

    # Invoke the C++ search component
    search_exit_code = run_search(build_dir, options, CPP_EXTRA_OPTIONS)

    return search_exit_code
