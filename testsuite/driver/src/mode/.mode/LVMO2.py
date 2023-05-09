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

from api import *

LVMO2 = {
    "compile": [
        MapleDriver(
        maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
        infiles=["${APP}.c"],
        outfile="${APP}.o",
        include_path=[
            "${MAPLE_BUILD_OUTPUT}/lib/include",
            "${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
            "${OUT_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include",
            "../lib"
        ],
        option="-O2 -g -fPIC -c"
        )
    ],
    "link":[
        MapleDriver(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            infiles=["${APP}.o"],
            outfile="${APP}.out",
            option="-static -L../lib -std=c99 -s -lst -lm"
        )
    ],
    "run": [
        Shell(
            "${OUT_ROOT}/tools/bin/qemu-aarch64 -L ${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc ${APP}.out > output.log 2>&1; echo exit $? >> output.log"
        )
    ],
    "verify":[
        Shell(
            "../tools/fpcmp-target -r 0.001 output.log expected.txt"
        )
    ]
}