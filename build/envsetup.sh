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
export MAPLE_ROOT=${curdir}
export SPEC=${MAPLE_ROOT}/testsuite/c_test/spec_test
export LD_LIBRARY_PATH=${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/lib:${MAPLE_ROOT}/tools/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-18.04/lib:${LD_LIBRARY_PATH}
export SPECPERLLIB=${SPEC}/bin/lib:${SPEC}/bin:${SPEC}/SPEC500-perlbench_r/data/all/input/lib:${SPEC}/SPEC500-perlbench_r/t/lib
export CASE_ROOT=${curdir}/testsuite
export OUT_ROOT=${curdir}/output
export ANDROID_ROOT=${curdir}/android
export MAPLE_BUILD_CORE=${MAPLE_ROOT}/build/core
if [ -d ${MAPLE_ROOT}/src/ast2mpl ]; then
  export IS_AST2MPL_EXISTS=1
else
  export IS_AST2MPL_EXISTS=0
fi
export GCOV_PREFIX=${MAPLE_ROOT}/report/gcda
export GCOV_PREFIX_STRIP=7

# display OS version
lsb_release -d

export TOOL_BIN_PATH=${MAPLE_ROOT}/tools/bin
if [ -d ${MAPLE_ROOT}/testsuite/driver/.config ];then
  rm -rf ${MAPLE_ROOT}/testsuite/driver/config
  rm -rf ${MAPLE_ROOT}/testsuite/driver/src/api
  rm -rf ${MAPLE_ROOT}/testsuite/driver/src/mode
  cd ${MAPLE_ROOT}/testsuite/driver
  ln -s -f .config config
  cd ${MAPLE_ROOT}/testsuite/driver/src
  ln -s -f .api api
  ln -s -f .mode mode
fi

cd ${MAPLE_ROOT}

OS_VERSION=`lsb_release -r | sed -e "s/^[^0-9]*//" -e "s/\..*//"`
if [ "$OS_VERSION" = "16" ] || [ "$OS_VERSION" = "18" ]; then
  export OLD_OS=1
else
  export OLD_OS=0
fi

# support multiple ARCH and BUILD_TYPE

if [ $1 = "arm" ]; then
  PLATFORM=aarch64
  USEOJ=0
elif [ $1 = "riscv" ]; then
  PLATFORM=riscv64
  USEOJ=0
elif [ $1 = "engine" ]; then
  PLATFORM=ark
  USEOJ=1
elif [ $1 = "ark" ]; then
  PLATFORM=ark
  USEOJ=1
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

export MAPLE_DEBUG=${DEBUG}
export TARGET_PROCESSOR=${PLATFORM}
export TARGET_SCOPE=${TYPE}
export USE_OJ_LIBCORE=${USEOJ}
export TARGET_TOOLCHAIN=clang
export MAPLE_BUILD_TYPE=${TARGET_PROCESSOR}-${TARGET_TOOLCHAIN}-${TARGET_SCOPE}
echo "Build:          $MAPLE_BUILD_TYPE"
export MAPLE_BUILD_OUTPUT=${MAPLE_ROOT}/output/${MAPLE_BUILD_TYPE}
export MAPLE_EXECUTE_BIN=${MAPLE_ROOT}/output/${MAPLE_BUILD_TYPE}/bin
export TEST_BIN=${CASE_ROOT}/driver/script
export PATH=$PATH:${MAPLE_EXECUTE_BIN}:${TEST_BIN}

if [ ! -f $MAPLE_ROOT/tools/qemu/usr/bin/qemu-aarch64 ] && [ "$OLD_OS" = "0" ]; then
  echo " "
  echo "!!! please run \"make setup\" to get proper qemu-aarch64"
  echo " "
fi



