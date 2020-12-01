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

curdir=$(pwd)
unset MAPLE_ROOT
export MAPLE_ROOT=${curdir}
unset ANDROID_ROOT
export ANDROID_ROOT=${curdir}/android
unset MAPLE_BUILD_CORE
export MAPLE_BUILD_CORE=${MAPLE_ROOT}/build/core
export PATH=$PATH:${MAPLE_ROOT}/output/bin
unset IS_AST2MPL_EXISTS
if [ -d ${MAPLE_ROOT}/src/ast2mpl ]; then
  export IS_AST2MPL_EXISTS=1
else
  export IS_AST2MPL_EXISTS=0
fi
export GCOV_PREFIX=${MAPLE_ROOT}/report/gcda
export GCOV_PREFIX_STRIP=7
