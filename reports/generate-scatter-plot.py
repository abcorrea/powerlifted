#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Create a scatter plot using matplotlib from two input file containing a single
# column of numerical data each

import os
import sys
from typing import Union, Iterable

import matplotlib.pyplot as plt
import numpy as np
from numpy.core.multiarray import ndarray

import timers


def generate_curve_plot(info):
    """
    Generate and scatter plot.
    :param info: list containing pairs of numbers
    :return: void
    """
    # plt.scatter plots dots, plt.plot plots a line
    x = np.array([int(x) for x, _ in info])
    y = np.array([int(y) for _, y in info])

    # Set scatter plot with color schema
    fig, ax = plt.subplots()
    ax.scatter(x,y, c=(x-y), cmap='coolwarm')

    # Sets up x=y line
    lims = [
        np.min([ax.get_xlim(), ax.get_ylim()]),  # min of both axes
        np.max([ax.get_xlim(), ax.get_ylim()]),  # max of both axes
    ]
    ax.plot(lims, lims, 'k-', alpha=0.75, zorder=0)
    ax.set_aspect('equal')
    ax.set_xlim(lims)
    ax.set_ylim(lims)

    plt.xlabel("X")
    plt.ylabel("Y")
    plt.show()

def parse(f):
    """
    Parse single column file into a list
    :param f: file
    :return: list where each element is a row-value of
    """
    info = []
    for line in f:
        v = line.split()
        info.append(v)
    return info


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: ./generate-curve-plot.py FILE")
        print("\t where FILE is a single column file.")
        sys.exit()

    timer = timers.Timer()
    info = []

    with timers.timing("Parsing files"):
        filename = sys.argv[1]
        if os.path.exists(filename):
            with open(filename, 'r') as f:
                try:
                    info = parse(f)
                except IOError:
                    print("Files could not be opened or read.")
                    sys.exit(-1)
        else:
            print("File %s does not exist" % filename)
            sys.exit(-1)

    generate_curve_plot(info)