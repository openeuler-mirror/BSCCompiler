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

CO2_BPL_C2M = {
    "compile": [
        Shell(
          "${MAPLE_ROOT}/tools/bin/clang2mpl ${APP}.c -- -isystem ${MAPLE_BUILD_OUTPUT}/lib/include -isystem ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include -isystem ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include -isystem ../lib/include --target=aarch64-linux-elf -Wno-return-type -U__SIZEOF_INT128__"
        ),
        Maple(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            run=["me", "mpl2mpl", "mplcg"],
            option={
                "me": "-O2 --quiet",
                "mpl2mpl": "-O2 --quiet",
                "mplcg": "-O2 --fPIC --quiet"
            },
            global_option="",
            infiles=["${APP}.bpl"]
        ),
        CLinker(
            infiles=["${APP}.s"],
            front_option="",
            outfile="${APP}.out",
            back_option="-lm"
        )
    ],
    "run": [
        Shell(
            "${MAPLE_ROOT}/tools/bin/qemu-aarch64 -L ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc ${APP}.out > output.log 2>&1"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        )
    ]
}
