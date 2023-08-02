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
check_env()
{
  if [ ! -n "${MAPLE_ROOT}" ];then
     echo "plz cd to maple root dir and source build/envsetup.sh arm release/debug"
     exit -2
  fi
 
  if [ ! -f "${BIN_MPLFE}" ] || [ ! -f "${BIN_MAPLE}" ];then
     echo "plz make to ${MAPLE_ROOT} and make tool chain first!!!"
     exit -3
  fi
}
# $1 cfilePath
# $2 cfileName
generate_ast()
{
  set -x
  $BIN_CLANG ${CFLAGS} -I ${MAPLE_BUILD_OUTPUT}/lib/include -I ${MAPLE_ROOT}/tools/clang+llvm-15.0.4-x86_64-linux-gnu-ubuntu-18.04-enhanced/lib/clang/15.0.4/include  --target=x86_64 -U __SIZEOF_INT128__ -emit-ast $1 -o "$2.ast"
  check_error
  set +x
}
 
# $1 cfileName
generate_mpl()
{
  set -x
  ${BIN_MPLFE} "$1.ast" -o "${1}_mplfe.mpl"
  check_error
  set +x
}
 
# $1 cfileName
generate_s()
{
  set -x
  ${BIN_MAPLE} ${OPT_LEVEL} ${DEBUG_OPTION} --mplcg-opt="--verbose-asm --verbose-cg" --genVtableImpl --save-temps "${1}_mplfe.mpl"
  check_error
  set +x
}
 
# $@ sFilePaths
link()
{
  set -x
  $BIN_CLANG ${OPT_LEVEL} ${LINK_OPTIONS} -o $OUT $@
  check_error
  set +x
}
parse_cmdline()
{
 while [ -n "$1" ]
 do
   OPTIONS="$(echo "$1" | sed 's/\(.*\)=\(.*\)/\1/')"
   PARAM="$(echo "$1" | sed 's/.*=//')"
   case "$OPTIONS" in
   out) OUT=$PARAM;;
   ldflags) LINK_OPTIONS=$PARAM;;
   optlevel) OPT_LEVEL=$PARAM;;
   debug) DEBUG=$PARAM;;
   cflags) CFLAGS=$PARAM;;
   help) help;;
   *) if [ "$(echo "$1" | sed -n '/.*=/p')" ];then
        echo "Error!!! the parttern \"$OPTIONS=$PARAM\" can not be recognized!!!"
        help;
      fi
      break;;
   esac
   shift
 done
 files=$@
 if [ ! -n "$files" ];then
    help
 fi
}
