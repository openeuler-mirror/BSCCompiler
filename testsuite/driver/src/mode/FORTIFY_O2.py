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
            option="-O2 -D_FORTIFY_SOURCE=2 -fPIC -S --save-temps",
            redirection="compile.log"
        )
]
link_part = [
        Shell(
            "${MAPLE_BUILD_OUTPUT}/bin/maple -L${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc  -std=c89 -s -o ${APP}.out `find *.s` -lm >tmp.log 2>&1 && (cat tmp.log | tee -a compile.log && rm tmp.log) || (cat tmp.log | tee -a compile.log && rm tmp.log && exit 1)"
        )
]

run_part = [
        Shell(
            "${OUT_ROOT}/tools/bin/qemu-aarch64 -L ${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc ${APP}.out > output.log 2>&1"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        ),
]
FORTIFY_O2 = {
    "compile": compile_part + link_part,
    "run": run_part,
    "compile_err":
        [Shell("EXPECT_ERR_START")] +
        compile_part + link_part +
        [Shell("EXPECT_ERR_END")],
    "run_err":
        [Shell("EXPECT_ERR_START")] +
        run_part +
        [Shell("EXPECT_ERR_END")]
}