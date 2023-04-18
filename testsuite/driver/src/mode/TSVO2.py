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

TSVO2 = {
    "compile": [
        MapleDriver(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            infiles=["${APP}.c"],
            outfile="${APP}.o",
            include_path=[
                "${MAPLE_BUILD_OUTPUT}/lib/include",
                "${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
                "${OUT_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include",
                "../lib/include"
            ],
            option="-O2 -DC_ENHANCED -std=c99 -fPIC -I ../lib/include -c",
        )
    ],
    "link": [
        MapleDriver(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            infiles=["${APP}"],
            outfile="a.exe",
            option="-std=gnu99 -no-pie -std=c99 -lm",
        )
    ],
    "run": [
        Shell(
            "${OUT_ROOT}/tools/bin/qemu-aarch64 -L ${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc a.exe | awk '{print $NF}' > output.log 2>&1"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        )
    ]
}
