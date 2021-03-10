Bootstrap: docker
From: ubuntu:18.04

%files
    `pwd` planning/powerlifted

%post
    ##### INSTALLATION DEPENDENCIES #####
    ## Install all necessary dependencies
    apt-get update && apt-get install --no-install-recommends -y \
            autotools-dev \
            bison \
            ca-certificates \
            flex \
            g++       \
            gdb \
            cmake \
            autotools-dev make automake autoconf     \
            git \
            python \
            python3 \
            libboost-all-dev=1.65.1.0ubuntu1 \


    ##### PLANNER INSTALLATION #####
    ## Compile the planner
    cd planning/powerlifted
    ./build.py
    chmod +w /planning/powerlifted

%runscript
        CURRENT_DIR=`pwd`

        DOMAINFILE=$1
        PROBLEMFILE=$2
        SEARCH=$3
        EVALUATOR=$4

        echo "Domain file:" ${DOMAINFILE}
        echo "Instance file:" ${PROBLEMFILE}

        /planning/powerlifted/powerlifted.py $@


%labels
## Please use the same field names and use only one line for each value.
Name        Powerlifted - ICAPS 2021
Description Planner used for the experiments reported in the paper "Delete-Relaxation Heuristics for Lifted Classical Planning" (Correa et al. ICAPS 2021)
Authors     Augusto B. Correa, Guillem Frances, Florian Pommerening, Malte Helmert