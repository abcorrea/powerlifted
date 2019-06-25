#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Create a histogram using matplotlib from an input file containing a single
# column of numerical data

import os
import sys

import matplotlib.pyplot as plt

import timers


def generate_histogram(info):
    """
    Generate and plot histogram. Histogram is showed in the screen. Labels
    and numbers of bin are handcrafted.
    :param info: list containing all data values
    :return: void
    """

    # Tweak parameters as needed.  The main parameters to be modified are
    # the labels and the number of bins.  Colors might also be modified.
    plt.hist(info, bins=10)
    plt.ylabel("Insert your y-label here.")
    plt.show()


def parse(f):
    """
    Parse single column file into a list
    :param f: file
    :return: list where each element is a row of f
    """
    return [int(line.rstrip('\n')) for line in f]


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: ./generate-histogram.py FILE")
        print("\t where file must be a one single column file")
        sys.exit()

    timer = timers.Timer()
    filename = sys.argv[1]
    info = []

    with timers.timing("Parsing file"):
        if os.path.exists(filename):
            with open(filename, 'r') as f:
                try:
                    info = parse(f)
                except IOError:
                    print("File could not be opened or read.")
                    sys.exit(-1)
        else:
            print("File %s does not exist" % filename)
            sys.exit(-1)

    with timers.timing("Generating histogram"):
        generate_histogram(info)
