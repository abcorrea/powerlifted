#! /usr/bin/env python
# -*- coding: utf-8 -*-
import os
import sys
from collections import defaultdict

import numpy as np
from plots import generate_curve_plot, generate_scatter_plot


# Gets two tables with initial columns key = (domain, problem) and plots the
# columns corresponding to (key, attribute for first file, attribute for
# second file).
# First column after key has index 0.

def split_table(f, r):
    """
    Create dict from table file where each key is assigned to the first two
    attributes of each row.

    :param f: file
    :return: dict with key corresponding to first two attributes of each line
    """
    for line in f:
        l = line.rstrip('\n').split()
        key = (l[0], l[1])
        r[key].append(tuple(l[2:]))


def extract_file_info(filename, r):
    if os.path.exists(filename):
        with open(filename, 'r') as f:
            try:
                info = split_table(f, r)
            except IOError:
                print("File could not be opened or read.")
                sys.exit(-1)
    else:
        print("File %s does not exist" % filename)
        sys.exit(-1)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: ./merge-two-tables.py I FILE1 FILE2")
        print(
            "\t where I represents the ith column of interest to be merged "
            "from FILE1 and FILE2. First column after the key has index 0.")
        sys.exit()

    attr = int(sys.argv[1])

    file1 = sys.argv[2]
    file2 = sys.argv[3]

    r = defaultdict(list)
    extract_file_info(file1, r)
    extract_file_info(file2, r)

    x = []
    y = []
    for key, data in r.items():
        if len(data) == 2:
            x.append(float(data[0][attr]))
            y.append(float(data[1][attr]))

    # Order based on attribute
    sorted_l = [(i,j) for i,j in sorted(zip(x,y), key=lambda n : n[0])]
    x = np.array([x for x, y in sorted_l])
    y = np.array([y for x, y in sorted_l])

    ylab = 'Sparse Representation'
    xlab = 'Types + Reachability Analysis'
    generate_scatter_plot(x,y, xlab, ylab, color=False)
