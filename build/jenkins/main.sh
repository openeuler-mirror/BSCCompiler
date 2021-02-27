#!/bin/bash

# Note:copy main.sh to the MAPLE_ROOT directory.

sample_list="exceptiontest helloworld iteratorandtemplate polymorphismtest rccycletest threadtest"
opt=O2

function debug_test {
    source build/envsetup.sh arm debug

    make clean

    make
    if [[ $? != 0 ]]; then
        exit 1
    fi

    make irbuild
    if [[ $? != 0 ]]; then
        exit 1
    fi

    make mplfe
    if [[ $? != 0 ]]; then
        exit 1
    fi
}

function release_test {
    source build/envsetup.sh arm release

    make clean

    make
    if [[ $? != 0 ]]; then
        exit 1
    fi

    make irbuild
    if [[ $? != 0 ]]; then
        exit 1
    fi

    make mplfe
    if [[ $? != 0 ]]; then
        exit 1
    fi

    make libcore OPT=${opt}
    if [[ $? != 0 ]]; then
        exit 1
    fi

    for dir in $sample_list
    do
        cd $MAPLE_ROOT/samples/$dir
        make OPT=O0
        if [[ $? != 0 ]]; then
            exit 1
        fi
        make clean

        make OPT=O2
        if [[ $? != 0 ]]; then
            exit 1
        fi
        make clean
    done

    if [[ $opt != "O2" ]]; then
        cp $MAPLE_BUILD_OUTPUT/ops/host-x86_64-${opt} $MAPLE_BUILD_OUTPUT/ops/host-x86_64-O2 -rf
    fi

    cd $MAPLE_ROOT
    make testall
    if [[ $? != 0 ]]; then
        exit 1
    fi
}

function main {
    debug_test

    release_test
}

main $@
