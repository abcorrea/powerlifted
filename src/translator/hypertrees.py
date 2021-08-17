#! /usr/bin/env python

import os
import subprocess


def get_hypertree_decompositions(task):
    print("Using Hypertree decompositions. 'BalancedGo' is expected to be in the PATH.")
    delete_previous_htd_files()
    for action in task.actions:
        i = 0
        f_name = generate_action_hypertree(action, i)
        compute_decompositions(f_name)
    delete_files(".ast")


def delete_previous_htd_files():
    print("Deleting previous '.htd' files.")
    delete_files(".htd")


def delete_files(extension):
    cwd = os.getcwd()
    files = os.listdir(cwd)
    for f in files:
        if f.endswith(extension):
            os.remove(os.path.join(cwd, f))


def generate_action_hypertree(action, i):
    f = open(action.name + ".ast", 'w')
    for p in action.precondition.parts:
        if p.predicate == '=':
            continue
        atom_name = "{}-{}".format(p.predicate, str(i))
        i = i + 1
        terms = ','.join(p.args).replace('?', '')
        f.write('%s(%s)\n' % (atom_name, terms))
    f.close()
    return f.name


def compute_decompositions(file):
    import sys
    cwd = os.getcwd()
    abs_path = os.path.join(cwd, file)

    decomp_file_name = file
    decomp_file_name = decomp_file_name.replace('.ast', '.htd')
    f = open(decomp_file_name, 'w')
    BALANCED_GO_CMD = ['BalancedGo', '-bench', '-approx', '10', '-det', '-graph', abs_path]
    subprocess.call(BALANCED_GO_CMD, stdout=f)