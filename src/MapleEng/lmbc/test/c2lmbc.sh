#!/bin/bash
# Copyright (C) [2022] Futurewei Technologies, Inc. All rights reverved.
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
set -x
[[ -z "${MAPLE_ROOT}" ]] && { echo "Please source OpenArkCompiler environment"; exit 1; }

if [ ! $# -le 2 ] || [ $# -eq 0 ]; then
  echo "usage: ./c2lmbc.sh <filename>.c [clang2mpl|hir2mpl]"
  exit
fi

if [ $# -le 1 ]; then
  FE=clang2mpl
else
  FE=$2
fi

# hir2mpl flags
CLANGBIN=$MAPLE_ROOT/output/tools/bin
LINARO=$MAPLE_ROOT/output/tools/gcc-linaro-7.5.0
ISYSTEM_FLAGS="-isystem $MAPLE_ROOT/output/aarch64-clang-release/lib/include -isystem $LINARO/aarch64-linux-gnu/libc/usr/include -isystem $LINARO/lib/gcc/aarch64-linux-gnu/7.5.0/include"
CLANGFE_FLAGS="-emit-ast --target=aarch64 -U __SIZEOF_INT128__ $ISYSTEM_FLAGS"
CLANGFE_FLAGS="$CLANGFE_FLAGS -o ${file%.c}.ast"
# clang2mpl flags
FE_FLAG="--ascii --simple-short-circuit --improved-issimple"
CLANG_FLAGS="--target=aarch64-linux-elf -Wno-return-type -U__SIZEOF_INT128__ -O3 -Wno-implicit -save-temps -fno-builtin-printf -fno-common -falign-functions=4"
# executables
CLANG=$CLANGBIN/clang
HIR2MPL=$MAPLE_EXECUTE_BIN/hir2mpl
CLANG2MPL=$MAPLE_EXECUTE_BIN/clang2mpl
MAPLE=$MAPLE_EXECUTE_BIN/maple
IRBUILD=$MAPLE_EXECUTE_BIN/irbuild
MPLSH=$MAPLE_EXECUTE_BIN/mplsh-lmbc

file=$(basename -- "$1")
file="${file%.*}"

if [[ $FE == "hir2mpl" ]]; then
  $CLANG $CLANGFE_FLAGS -o ${file%.c}.ast $file.c || exit 1
  $HIR2MPL ${file%.c}.ast -enable-variable-array -o ${file%.c}.mpl || exit 1
else
  $CLANG2MPL $FE_FLAG $file.c -- $CLANG_FLAGS || exit 1
fi

$MAPLE --run=me --option="-O2 --no-lfo --no-mergestmts --skip-phases=slp" --genmempl --genlmbc $file.mpl
$IRBUILD $file.lmbc
mv comb.me.mpl $file.me.mpl
