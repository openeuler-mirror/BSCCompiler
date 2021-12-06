#!/bin/bash
# Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reverved.
#
# Licensed under the Mulan Permissive Software License v2
# You can use this software according to the terms and conditions of the MulanPSL - 2.0.
# You may obtain a copy of MulanPSL - 2.0 at:
#
#   https://opensource.org/licenses/MulanPSL-2.0
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the MulanPSL - 2.0 for more details.
#

set -e

[ -n "$MAPLE_ROOT" ] || { echo MAPLE_ROOT not set. Please source envsetup.sh.; exit 1; }

CURRDIR=`pwd`
rel=`realpath --relative-to=$MAPLE_ROOT $CURRDIR`

dir=$1
src=$2
opt=$3
if [ $# -le 3 ]; then
  verbose=0
else
  verbose=$4
fi

WORKDIR=$MAPLE_BUILD_OUTPUT/$rel/$dir/aarch64_with_hir2mpl

mkdir -p $WORKDIR
cp $dir/$src.c $WORKDIR
cd $WORKDIR

echo ======================================================================== > cmd.log
echo ====================== use hir2mpl as C Frontend ========================= >> cmd.log
echo ======================================================================== >> cmd.log
echo cd $WORKDIR >> cmd.log

echo $MAPLE_ROOT/tools/bin/clang -emit-ast -o $src.ast $src.c >> cmd.log
$MAPLE_ROOT/tools/bin/clang -emit-ast -o $src.ast $src.c >> doit.log 2>&1

echo $MAPLE_EXECUTE_BIN/hir2mpl $src.ast -o $src.mpl >> cmd.log
$MAPLE_EXECUTE_BIN/hir2mpl $src.ast -o $src.mpl >> doit.log 2>&1

if [ $opt -eq 0 ]; then
  echo $MAPLE_EXECUTE_BIN/maple --run=mplcg --option=\"-quiet\" $src.mpl >> cmd.log
  $MAPLE_EXECUTE_BIN/maple --run=mplcg --option="-quiet" $src.mpl >> doit.log 2>&1
else
  echo $MAPLE_EXECUTE_BIN/maple --run=mplcg --option=\"-O2 -quiet\" $src.mpl >> cmd.log
  $MAPLE_EXECUTE_BIN/maple --run=mplcg --option="-O2 -quiet" $src.mpl >> doit.log 2>&1
fi

LINARO=$MAPLE_ROOT/tools/gcc-linaro-7.5.0

echo $LINARO/bin/aarch64-linux-gnu-gcc -o $src.out $src.s >> cmd.log
$LINARO/bin/aarch64-linux-gnu-gcc -o $src.out $src.s

echo $MAPLE_ROOT/tools/bin/qemu-aarch64 -L $LINARO/aarch64-linux-gnu/libc $src.out >> cmd.log
$MAPLE_ROOT/tools/bin/qemu-aarch64 -L $LINARO/aarch64-linux-gnu/libc $src.out > output.log

cat cmd.log >> allcmd.log

if [ $verbose -eq 1 ]; then
  cat cmd.log
  cat output.log
fi

