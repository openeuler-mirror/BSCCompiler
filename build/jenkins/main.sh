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
  make irbuild_test
  make c_test
}

function release_test {
  source build/envsetup.sh arm release

  if [ ! -d $MAPLE_ROOT/third_party/ctorture ]; then
    cd $TOOLS
    rm -rf ctorture $MAPLE_ROOT/third_party/ctorture
    git clone --depth 1 https://gitee.com/hu-_-wen/ctorture.git
    mv ctorture $MAPLE_ROOT/third_party/
    echo Downloaded ctorture.
  fi
  cd $MAPLE_ROOT/third_party/ctorture
  git reset --hard
  git clean -fd
  git pull

  cd $MAPLE_ROOT
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

  make ctorture-ci
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
