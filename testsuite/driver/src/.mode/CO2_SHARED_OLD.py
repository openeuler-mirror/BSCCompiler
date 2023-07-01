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

compile_part = [
        C2ast(
            clang="${MAPLE_ROOT}/tools/bin/clang",
            include_path=[
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include",
                "../lib/include"
            ],
            option="--target=aarch64 -U __SIZEOF_INT128__",
            infile="${APP}.c",
            outfile="${APP}.ast"
        ),
        Hir2mpl(
            hir2mpl="${MAPLE_BUILD_OUTPUT}/bin/hir2mpl ",
            infile="${APP}.ast",
            outfile="${APP}.mpl"
        ),
        Maple(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            run=["me", "mpl2mpl", "mplcg"],
            option={
                "me": "-O2 --quiet",
                "mpl2mpl": "-O2 --quiet",
                "mplcg": " -fPIC -O2 --quiet"
            },
            global_option="-g",
            infiles=["${APP}.mpl"],
            redirection="compile.log"
        )
]

link_part = [
         CLinker(
           infiles=["${APP}_main.s"],
           front_option="",
           outfile="main.exe",
           back_option="-L. -l${APP} -lm -lpthread"
         )
]

generate_shared_lib = [
         CLinker(
           infiles=["${APP}.s"],
           front_option="-shared -fPIC",
           outfile="lib${APP}.so",
           back_option="-lm -lpthread"
         )
]


run_part = [
        Shell(
            "LD_LIBRARY_PATH=. ${MAPLE_ROOT}/tools/bin/qemu-aarch64 -L ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc main.exe > output.log 2>&1"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        ),
]
CO2_SHARED_OLD = {
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
