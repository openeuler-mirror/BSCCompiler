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

compile_part = [
    MapleDriver(
        maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
        infiles=["${APP}.c"],
        outfile="${APP}.s",
        include_path=[
                "${MAPLE_BUILD_OUTPUT}/lib/include",
                "${MAPLE_BUILD_OUTPUT}/lib/libc_enhanced/include",
                "${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
                "${OUT_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include",
                "../lib/include"
        ],
        option="-O2 --npe-check-dynamic --npe-check-dynamic-all -fPIC --save-temps -S",
        redirection="compile.log"
    )
]

link_part = [
        Shell(
        "${MAPLE_BUILD_OUTPUT}/bin/maple  -o a.out `find *.s`  -lm>tmp.log 2>&1 && (cat tmp.log | tee -a compile.log && rm tmp.log) || (cat tmp.log | tee -a compile.log && rm tmp.log && exit 1)"
        )
]

run_part = [
        Shell(
            "${OUT_ROOT}/tools/bin/qemu-aarch64 -L ${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc a.out > output.log 2>&1"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
	    ),
]

ENCO2_N_D_ALL = {
    "compile": compile_part + link_part,
    "run": run_part,
    "compile_err":
        [Shell("EXPECT_ERR_START")] +
        compile_part + link_part +
        [Shell("EXPECT_ERR_END")],
    "run_err":
        [Shell("EXPECT_ERR_START")] +
        run_part +
        [Shell("EXPECT_ERR_END")],
}