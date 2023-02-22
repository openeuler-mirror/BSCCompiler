#
# Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

compile_part = [
    MapleDriver(
        maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
        infiles=["${APP}.c"],
        outfile="${APP}.s",
        include_path=[
            "${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
            "${OUT_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include",
            "../lib/include"
        ],
        option="-O2 -g -fPIC -S",
        redirection="compile.log"
        )
]

link_part = [
    MapleDriver(
        maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
        infiles=["${APP}_main.s"],
        outfile="main.exe",
        option="-L. -l${APP} -lm -lpthread"
    )
]

generate_shared_lib = [
    MapleDriver(
        maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
        infiles=["${APP}.s"],
        outfile="lib${APP}.so",
        option="-shared -fPIC -lm -lpthread"
    )
]

run_part = [
        Shell(
            "LD_LIBRARY_PATH=. ${OUT_ROOT}/tools/bin/qemu-aarch64 -L ${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc main.exe > output.log 2>&1"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        ),
]
CO2_SHARED = {
    "compile": compile_part,
    "linkmain":link_part,
    "generatesharelib":generate_shared_lib,
    "run": run_part,
    "compile_err":
        [Shell("EXPECT_ERR_START")] +
        compile_part +
        [Shell("EXPECT_ERR_END")],
    "run_err":
        [Shell("EXPECT_ERR_START")] +
        run_part +
        [Shell("EXPECT_ERR_END")]
}