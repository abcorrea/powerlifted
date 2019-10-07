#!/usr/bin/env bash

for i in runs-*; do
    for run in $i/*; do
        if [ -f $run/sas_plan ]; then
            echo $run/*pddl >> test_here
        fi
    done
done
