#!/usr/bin/env bash

for i in */; do
    ulimit -v 16777216
    ulimit -m 16777216
    ulimit -t 300
    cd $i
    ./search output.lifted naive blind full_reducer > run.out 2> run.err
    cd ..
done
