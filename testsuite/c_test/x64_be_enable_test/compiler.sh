#!/bin/bash
#
# Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
set -e

check_error()
{
  error_no=$?
  if [ "$error_no" != 0 ];then
    exit $error_no
  fi
}

init_env()
{
  if [ -f multiprocess.sh ]; then
    source multiprocess.sh
    MultiProcessInit
  fi
}

init_parameter()
{
 OUT="a.out"
 LINK_OPTIONS=""
 BIN_MPLFE=${MAPLE_BUILD_OUTPUT}/bin/hir2mpl
 BIN_MAPLE=${MAPLE_BUILD_OUTPUT}/bin/maple
 BIN_CLANG=${MAPLE_ROOT}/tools/clang+llvm-15.0.4-x86_64-linux-gnu-ubuntu-18.04-enhanced/bin/clang
 OPT_LEVEL="-O0"
 DEBUG="false"
 CFLAGS=""
}

prepare_options()
{
 if [ "${DEBUG}" == "true" ]; then
   # inline may delete func node and didn't update debug info, it'll cause debug info error when linking
   DEBUG_OPTION="-g --mpl2mpl-opt=--skip-phase=inline"
 fi
}

help()
{
  echo
  echo "USAGE"
  echo "    ./compile.sh [options=...] files..."
  echo
  echo "EXAMPLE"
  echo "    ./compile.sh out=test.out ldflags=\"-lm -pthread\" test1.c test2.c"
  echo
  echo "OPTIONS"
  echo "    out:           binary output path, default is a.out"
  echo "    ldflags:       ldflags"
  echo "    optlevel:      -O0 -O1 -O2(default)"
  echo "    debug:         true(-g), false(default)"
  echo "    cflags:        cflags"
  echo "    help:          print help"
  echo
  exit -1
}

main()
{
 init_env
 init_parameter
 check_env
 parse_cmdline $@
 prepare_options
 s_files="$(echo ${files}|sed 's\\.c$\_mplfe.s\g')"
 for i in $files
 do
   file_name=${i//.c/}
   generate_ast $i $file_name
   generate_mpl $file_name
   generate_s $file_name
 done
 link $s_files
}

source ${MAPLE_ROOT}/testsuite/c_test/x64_be_enable_test/compiler_func.sh
main $@ 2>&1 | tee log.txt
