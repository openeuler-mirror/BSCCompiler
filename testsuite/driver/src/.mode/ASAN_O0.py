#
# Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
#
# OpenArkCompiler is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
# FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
#

# NOTE: you must compile maple with ENABLE_MAPLE_SAN=ON
# The value of ENABLE_MAPLE_SAN is hard coded as OFF in src/mapleall/CMakeList.txt
# Please change the value manually.

from api import *

ASAN_O0 = {
    "compile": [
      Driver(
        maple="${MAPLE_ROOT}/tools/bin/clang",
        global_option="-target aarch64-linux-gnu -isystem ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include -isystem ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include -U __SIZEOF_INT128__ -emit-ast",
        inputs="${APP}.c"
      ),
      Driver(
        maple="${MAPLE_BUILD_OUTPUT}/bin/hir2mpl",
        global_option="--enable-variable-array",
        inputs="${APP}.ast"
      ),
      MapleDriver(
        maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
        infiles=["${APP}.mpl"],
        outfile="${APP}.s",
        option="--run=me:mpl2mpl:mplcg --option=\"-O0 --san=0x5:-O0 --no-inline -quiet:-O0\""
      ),
      Driver(
        maple="${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/bin/aarch64-linux-gnu-gcc",
        global_option="-c",
        inputs="${APP}.s"
      ),
      Driver(
        maple="${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/bin/aarch64-linux-gnu-gcc",
        global_option="-lasan -lubsan -ldl -lpthread -lm -lrt",
        inputs="${APP}.o"
      )
    ],
    "run": [
        Shell(
            "${MAPLE_ROOT}/tools/bin/qemu-aarch64 -L ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc a.out > output.log 2>&1"
        )
    ]
}