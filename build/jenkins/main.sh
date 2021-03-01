#!/bin/bash

set -e

# Note:copy main.sh to the MAPLE_ROOT directory.

sample_list="exceptiontest helloworld iteratorandtemplate polymorphismtest rccycletest threadtest"
opt=O2

function debug_test {
    source build/envsetup.sh arm debug
    make clean
    make
    make irbuild
    make mplfe
}

function release_test {
    source build/envsetup.sh arm release

    make clean
    make
    make irbuild
    make mplfe
    make libcore OPT=${opt}

    for dir in $sample_list
    do
        cd $MAPLE_ROOT/samples/$dir
        make OPT=O0
        make clean

        make OPT=O2
        make clean
    done

    if [[ $opt != "O2" ]]; then
        cp $MAPLE_BUILD_OUTPUT/ops/host-x86_64-${opt} $MAPLE_BUILD_OUTPUT/ops/host-x86_64-O2 -rf
    fi

    cd $MAPLE_ROOT
    make testall
}

function main {
    # clean and setup env
    source build/envsetup.sh arm release
    make clobber
    make setup

    debug_test

    release_test
}

main $@
