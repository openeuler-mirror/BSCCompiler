#!/bin/bash
#
# Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#

function print_usage {
  echo " "
  echo "usage: source envsetup.sh arm/ark/engine/riscv release/debug"
  echo " "
}

if [ "$#" -lt 2 ]; then
  print_usage
# return
fi

curdir=$(pwd)
unset MAPLE_ROOT
export MAPLE_ROOT=${curdir}
unset ANDROID_ROOT
export ANDROID_ROOT=${curdir}/android
unset MAPLE_BUILD_CORE
export MAPLE_BUILD_CORE=${MAPLE_ROOT}/build/core
unset IS_AST2MPL_EXISTS
if [ -d ${MAPLE_ROOT}/src/ast2mpl ]; then
  export IS_AST2MPL_EXISTS=1
else
  export IS_AST2MPL_EXISTS=0
fi
export GCOV_PREFIX=${MAPLE_ROOT}/report/gcda
export GCOV_PREFIX_STRIP=7

# display OS version
lsb_release -d

OS_VERSION=`lsb_release -r | sed -e "s/^[^0-9]*//" -e "s/\..*//"`
if [ "$OS_VERSION" = "16" ] || [ "$OS_VERSION" = "18" ]; then
  OLD_OS=1
else
  OLD_OS=0
fi
export OLD_OS=${OLD_OS}

# workaround for current build
if [ "$#" -eq 0 ]; then
unset MAPLE_BUILD_OUTPUT
export MAPLE_BUILD_OUTPUT=${MAPLE_ROOT}/output

unset MAPLE_EXECUTE_BIN
export MAPLE_EXECUTE_BIN=${MAPLE_ROOT}/output/bin

unset MAPLE_DEBUG
export MAPLE_DEBUG=0
return
fi

# support multiple ARCH and BUILD_TYPE

if [ $1 = "arm" ]; then
  PLATFORM=aarch64
  USEOJ=0
elif [ $1 = "engine" ]; then
  PLATFORM=ark
  USEOJ=1
elif [ $1 = "ark" ]; then
  PLATFORM=ark
  USEOJ=1
elif [ $1 = "ark2" ]; then
  PLATFORM=ark
  USEOJ=2
elif [ $1 = "riscv" ]; then
  PLATFORM=riscv64
  USEOJ=0
else
  print_usage
  return
fi

if [ "$2" = "release" ]; then
  TYPE=release
  DEBUG=0
elif [ "$2" = "debug" ]; then
  TYPE=debug
  DEBUG=1
else
  print_usage
  return
fi

unset MAPLE_DEBUG
export MAPLE_DEBUG=${DEBUG}

unset TARGET_PROCESSOR
export TARGET_PROCESSOR=${PLATFORM}

unset TARGET_SCOPE
export TARGET_SCOPE=${TYPE}

unset USE_OJ_LIBCORE
export USE_OJ_LIBCORE=${USEOJ}

unset TARGET_TOOLCHAIN
export TARGET_TOOLCHAIN=clang

unset MAPLE_BUILD_TYPE
export MAPLE_BUILD_TYPE=${TARGET_PROCESSOR}-${TARGET_TOOLCHAIN}-${TARGET_SCOPE}
echo "Build:          $MAPLE_BUILD_TYPE"

unset MAPLE_BUILD_OUTPUT
export MAPLE_BUILD_OUTPUT=${MAPLE_ROOT}/output/${MAPLE_BUILD_TYPE}

unset MAPLE_EXECUTE_BIN
export MAPLE_EXECUTE_BIN=${MAPLE_ROOT}/output/${MAPLE_BUILD_TYPE}/bin

export PATH=$PATH:${MAPLE_EXECUTE_BIN}

if [ ! -f $MAPLE_ROOT/tools/qemu/package/usr/bin/qemu-aarch64 ] && [ "$OLD_OS" = "0" ]; then
  echo " "
  echo "!!! please run \"make setup\" to get proper qemu-aarch64"
  echo " "
fi
