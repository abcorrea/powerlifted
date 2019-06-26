#!/usr/bin/env bash

# Gets a file in the format
#
# > DOMAIN PROBLEM : initial state size XX : time [YYYs CPU, ZZZs wall-clock]
#
# and parses it into a file
#
# > domain problem state_size cpu_time user_time
# > DOMAIN PROBLEM XX         YYY      ZZZ
# > ...
#
# Usage
# $ ./clean-output.sh INPUT_FILE
#

while IFS= read -r line; do
    echo $line | sed "s/: initial state size//" | sed "s/: time \[//" | sed "s/s CPU,//" | sed "s/s wall-clock\]//"
done < "$1"
