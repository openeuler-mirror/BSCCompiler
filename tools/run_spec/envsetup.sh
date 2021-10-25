#!/bin/bash
# export LD_LIBRARY_PATH=$MAPLE_ROOT/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/lib
# spec
export SPEC=$HOME/spec2017
echo SPEC=$SPEC
cd ${SPEC}
source shrc
cd -
ulimit -s unlimited

if [ $# -eq 1 ]; then
  export SPEC_CFG=$1;
  echo "You choose $1 as the front end."
else
  export SPEC_CFG=mplfe
  echo "You choose mplfe as the front end."
fi

#echo 3 | sudo tee /proc/sys/vm/drop_caches
