#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Create a curve plot using matplotlib from an input file containing a single
# column of numerical data

import os
import sys

import matplotlib.pyplot as plt
import numpy as np

import timers

COLORS = ['r','g','b','c','m','y','k','o','p']
PLOT_INDEX = 0

def generate_curve_plot(info,name):
    """
    Generate and plot a curve graphic.  Curve is ordered according to the
    row order of the input column.
    :param info: list containing numerical values
    :return: void
    """

    # Transform creates auxiliary list
    x = [i for i in range(1, len(info) + 1)]

    # plt.scatter plots dots, plt.plot plots a line
    plt.scatter(x, info, marker="x")

def parse(f):
    """
    Parse single column file into a list
    :param f: file
    :return: list where each element is a row-value of
    """
    return [int(line.rstrip('\n')) for line in f]


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: ./generate-curve-plot.py N [FILES]")
        print("\t where N is the number of files that are passed in FILES")
        sys.exit()

    try:
        number_of_files = int(sys.argv[1])
    except ValueError:
        print("First parameter must be the number of files to be read.")
        print("Usage: ./generate-curve-plot.py N [FILES]")
        print("\t where N is the number of files that are passed in FILES")
        sys.exit(-1)

    if len(sys.argv) < number_of_files + 2:
        print("ERROR: first parameter must match the number of files passed")
        print("Usage: ./generate-curve-plot.py N [FILES]")
        print("\t where N is the number of files that are passed in FILES")
        sys.exit(-1)

    timer = timers.Timer()
    info = []

    with timers.timing("Parsing file"):
        for i in range(number_of_files):
            filename = sys.argv[i + 2]
            if os.path.exists(filename):
                with open(filename, 'r') as f:
                    try:
                        info = parse(f)
                        generate_curve_plot(info, filename)
                    except IOError:
                        print("File could not be opened or read.")
                        sys.exit(-1)
            else:
                print("File %s does not exist" % filename)
                sys.exit(-1)

    # Add legends.  This might also need a bit of handcraft.
    plt.legend(['Original', 'Modified'])
    plt.show()
