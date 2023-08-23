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
  echo "usage: source envsetup.sh arm/ark/engine/riscv/x86_64 release/debug"
  echo " "
}

if [ "$#" -lt 2 ]; then
  print_usage
# return
fi

curdir=$(pwd)
# force to use local cmake and ninja
export PATH=${curdir}/tools/cmake/bin:${curdir}/tools/ninja:$PATH

export MAPLE_ROOT=${curdir}
export SPEC=${MAPLE_ROOT}/testsuite/c_test/spec_test
# enhanced clang patch, LD_LIBRARY_PATH
export ENHANCED_CLANG_PATH=${MAPLE_ROOT}/tools/clang+llvm-15.0.4-x86_64-linux-gnu-ubuntu-18.04-enhanced
export PURE_CLANG_PATH=${MAPLE_ROOT}/tools/clang+llvm-15.0.4-x86_64-linux-gnu-ubuntu-18.04-pure
export LLVM_PATH=${MAPLE_ROOT}/third_party/llvm-15.0.4.src

export LD_LIBRARY_PATH=${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/lib:${ENHANCED_CLANG_PATH}/lib:${LD_LIBRARY_PATH}
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
elif [ $1 = "x86_64" ] ; then
  PLATFORM=x86_64
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
export BISHENGC_GET_OS_VERSION="$(lsb_release -i | awk '{print $3}')"
# Enable Autocompletion for maple driver
if [ -f $MAPLE_ROOT/tools/maple_autocompletion.sh ]; then
  source ${MAPLE_ROOT}/tools/maple_autocompletion.sh
fi

if [ ! -f $MAPLE_ROOT/tools/qemu/usr/bin/qemu-aarch64 ] && [ "$OLD_OS" = "0" ]; then
  echo " "
  echo "!!! please run \"make setup\" to get proper qemu-aarch64"
  echo " "
fi

function mm
{
  THREADS=$(cat /proc/cpuinfo| grep "processor"| wc -l)
  PWD=$(pwd)
  num=${#CASE_ROOT}
  let num++
  ALL_MODE_LIST=$(cd ${CASE_ROOT}/driver/src/mode; find -name "*.py" | xargs basename -s .py;)
  TARGET=${PWD:${num}}
  MODE=

  #mm MODE=O0
  if [ $# -lt 3 ] && [[ "x${1^^}" =~ ^xMODE=.* ]]; then
      MODE=${1#*=}
      MODE=${MODE^^}
      python3 ${CASE_ROOT}/driver/src/driver.py --target=${TARGET} --mode=${MODE} --detail
  elif [ $# -lt 3 ] && [[ `echo ${ALL_MODE_LIST[@]} | grep -w ${1^^}` ]] ; then
      MODE=${1^^}
      python3 ${CASE_ROOT}/driver/src/driver.py --target=${TARGET} --mode=${MODE} --detail

  #mm clean
  elif [ $# -lt 3 ] && [ "x${1}" = "xclean" ]; then
      python3 ${CASE_ROOT}/driver/src/driver.py --target=${TARGET} --clean --detail

  #mm save
  elif [ $# = 1 ] && [ "x${1}" = "xsave" ]; then
      python3 ${CASE_ROOT}/driver/src/driver.py --target=${TARGET} --save

  #mm testall
  elif [ $# = 1 ] && [ -f ${CASE_ROOT}/driver/config/${1}.conf ]; then
      TARGET=${1}
      python3 ${CASE_ROOT}/driver/src/driver.py --target=${TARGET} --run-path=${OUT_ROOT}/host/test --j=${THREADS}

  #mm testall MODE=O0
  elif [ $# = 2 ] && [ -f ${CASE_ROOT}/driver/config/${1}.conf ]; then
      if [[ "x${2^^}" =~ ^xMODE=.* ]]; then
          MODE=${2#*=}
          MODE=${MODE^^}
      elif [[ `echo ${ALL_MODE_LIST[@]} | grep -w ${2^^}` ]] ; then
          MODE=${2^^}
      fi
      TARGET=${1}
      python3 ${CASE_ROOT}/driver/src/driver.py --target=${TARGET} --run-path=${OUT_ROOT}/host/test --mode=${MODE} --j=${THREADS}

  #mm app_test
  elif [ $# = 1 ] && [ -d ${CASE_ROOT}/${1} ]
  then
      TARGET=${1}
      python3 ${CASE_ROOT}/driver/src/driver.py --target=${TARGET} --run-path=${OUT_ROOT}/host/test --j=${THREADS}

  #mm app_test MODE=O2
  elif [ $# = 2 ] && [ -d ${CASE_ROOT}/${1} ]; then
      if [[ "x${2^^}" =~ ^xMODE=.* ]]; then
          MODE=${2#*=}
          MODE=${MODE^^}
      elif [[ `echo ${ALL_MODE_LIST[@]} | grep -w ${2^^}` ]] ; then
          MODE=${2^^}
      fi
      TARGET=${1}
      python3 ${CASE_ROOT}/driver/src/driver.py --target=${TARGET} --run-path=${OUT_ROOT}/host/test --mode=${MODE} --j=${THREADS}

  elif [ $# = 1 ] && [ "x${1,,}" = "x-h" -o "x${1,,}" = "x--help" ];
  then
      cat <<EOF
---------------------- MAPLE TEST FRAME INSTRUCTION ---------------------------
[2021/9/17]: capable with uppercase and lowercase for MODE, and "MODE=" is not necessary.
[*] Run mm [command] under ${MAPLE_ROOT}:
  - mm testall:               run all test cases for all modes

  - mm testall (MODE=)XX      run all test cases with XX mode. type XX directly if XX in [MODE SUITE]
                              e.g. mm testall [co0 / CO0 / mode=co0 / MODE=CO0];

  - mm [testsuite]            run [testsuite] with all modes. note [testsuite] should start under $CASE_ROOT.
                              e.g. mm c_test/ast_test : test all testcases in c_test/ast_test
                              e.g. mm c_test : test all testcases in c_test

  - mm [testsuite] (MODE=)XX  run [testsuite] with XX mode. note [testsuite] should support XX mode.
                              e.g. mm c_test/gtorture_test [CO2 / co2 / MODE=CO2 / mode=co2]

[*] Run mm [command] under    single testcase (${MAPLE_ROOT}/testsuite/c_test/ast_test/AST0001-HelloWorld):
  - mm MODE=XX:               run XX mode for this case
                              e.g. mm [ASTO0 / asto0 / mode=asto0 / MODE=ASTO0]

  - mm save:                  save temp files for this case

  - mm clean:            clean all temp files for this case

EOF
  else
      echo "Input Wrong~! please run: mm -h/--help for more help!"
  fi
}

