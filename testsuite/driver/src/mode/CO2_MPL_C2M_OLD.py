#
# Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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

CO2_MPL_C2M_OLD = {
    "compile": [
        Shell(
          "${OUT_ROOT}/tools/bin/clang2mpl --ascii ${APP}.c -- -isystem ${MAPLE_BUILD_OUTPUT}/lib/include -isystem ${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include -isystem ${OUT_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include -isystem ../lib/include --target=aarch64-linux-elf -Wno-return-type -U__SIZEOF_INT128__"
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
            infiles=["${APP}.mpl"]
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
            "${OUT_ROOT}/tools/bin/qemu-aarch64 -L ${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc ${APP}.out > output.log 2>&1"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        )
    ]
}
