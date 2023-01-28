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
        C2ast(
            clang="${OUT_ROOT}/tools/bin/clang",
            include_path=[
                "${MAPLE_BUILD_OUTPUT}/lib/include",
                "${MAPLE_BUILD_OUTPUT}/lib/libc_enhanced/include",
                "${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
                "${OUT_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include",
                "../lib/include"
            ],
            option="--target=aarch64 -U __SIZEOF_INT128__ -DC_ENHANCED",
            infile="${APP}.c",
            outfile="${APP}.ast",
            redirection="compile.log"
        ),
        Hir2mpl(
            hir2mpl="${MAPLE_BUILD_OUTPUT}/bin/hir2mpl",
            option="-boundary-check-dynamic",
            infile="${APP}.ast",
            outfile="${APP}.mpl",
            redirection="compile.log"
        ),
        Maple(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            run=["me", "mpl2mpl", "mplcg"],
            option={
                "me": "-O2 --quiet",
                "mpl2mpl": "-O2",
                "mplcg": "-O2 --fPIC --quiet"
            },
            global_option="--save-temps --boundary-check-dynamic --boundary-arith-check --boudary-dynamic-call-fflush",
            infiles=["${APP}.mpl"],
            redirection="compile.log"
        )
]

link_part = [
        Shell(
            "${OUT_ROOT}/tools/bin/aarch64-linux-gnu-gcc -O2 -static -L../lib/include -std=c89 -s -o a.out `find *.s` -lm -lst>tmp.log 2>&1 && (cat tmp.log | tee -a compile.log && rm tmp.log) || (cat tmp.log | tee -a compile.log && rm tmp.log && exit 1)"
        )
]

run_part = [
        Shell(""),
        Shell(
            "${OUT_ROOT}/tools/bin/qemu-aarch64 -L ${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc a.out ${arg} > output.log 2>&1"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        ),
]

ENCO2_B_D_A_C = {
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
