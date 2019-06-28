#! /usr/bin/env python
# -*- coding: utf-8 -*-


import matplotlib.pyplot as plt
import numpy as np


def generate_scatter_plot(x, y, xlab, ylab, log=False, color=True):
    """
    Generate and scatter plot.
    :param x: np.array
    :param y: np.array
    :return: void
    """

    # Set scatter plot with color schema
    fig, ax = plt.subplots()
    if color:
        if log:
            color_schema = (np.log10(x) - np.log10(y))
        else:
            color_schema = (x-y)
    else:
        color_schema = None
    ax.scatter(x,y, c=color_schema, cmap='coolwarm', marker='x')
    if log:
        ax.set_xscale('log')
        ax.set_yscale('log')

    # Sets up x=y line
    lims = [
        np.min([ax.get_xlim(), ax.get_ylim()]),  # min of both axes
        np.max([ax.get_xlim(), ax.get_ylim()]),  # max of both axes
    ]
    ax.plot(lims, lims, 'k-', alpha=0.75, zorder=0)
    ax.set_aspect('equal')
    ax.set_xlim(lims)
    ax.set_ylim(lims)

    plt.xlabel(xlab, wrap=True)
    plt.ylabel(ylab, wrap=True)
    plt.show()

def generate_curve_plot(X, legends, log=False):
    """
    Generate and plot many curves.  Curve is ordered according to the
    row order of the input column.
    :param X: list of lists to be plotted
    :return: void
    """

    # Transform creates auxiliary list
    for info in X:
        x = [i for i in range(1, len(info) + 1)]

        # plt.scatter plots dots, plt.plot plots a line
        plt.scatter(x, info, marker="x")

    if log:
        plt.xscale('log')
        plt.yscale('log')
    plt.legend(legends)
    plt.show()
