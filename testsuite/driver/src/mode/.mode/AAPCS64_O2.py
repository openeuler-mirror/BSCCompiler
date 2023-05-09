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

AAPCS64_O2 = {
    "compile": [
        MapleDriver(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            infiles=["${APP}.c"],
            outfile="${APP}.s",
            include_path=[
                "${MAPLE_BUILD_OUTPUT}/lib/include"
            ],
            option="-O2 -fPIC -g -static -L../../lib/c -lst -lm --save-temps -S",
            redirection="compile.log"
        )
    ],
    "gcc_compile": [
        Shell(
            "${OUT_ROOT}/tools/bin/aarch64-linux-gnu-gcc \
            -isystem ${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include          \
            -isystem ${OUT_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include     \
            -U __SIZEOF_INT128__ -O2 -S ${APP}.c",
        ),
    ],
    "link_run": [
        CLinker(
            infiles=["${APP}"],
            front_option="-std=gnu99 -no-pie",
            outfile="a.out",
            back_option="-lm -L${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/lib/"
        ),
        Shell(
            "${OUT_ROOT}/tools/bin/qemu-aarch64 -L ${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc a.out > output.log 2>&1"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
	)
    ]
}
