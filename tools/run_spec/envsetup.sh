#!/bin/bash
export LD_LIBRARY_PATH=$MAPLE_ROOT/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/lib
# spec
export SPEC=$HOME/spec2017
echo SPEC=$SPEC
cd ${SPEC}
source shrc
cd -
ulimit -s unlimited

#echo 3 | sudo tee /proc/sys/vm/drop_caches
