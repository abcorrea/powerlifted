#!/usr/bin/env bash

# argument is the file with pairs "domain file"

while read -r domain inst; do
    base=$(dirname $domain)
    validate $domain $inst $base/sas_plan > $base.out 2> $base.out
done < $1
