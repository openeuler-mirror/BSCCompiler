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

CO0_PIE = {
    "compile_main": [
        C2ast(
            clang="${OUT_ROOT}/tools/bin/clang",
            include_path=[
                "${MAPLE_BUILD_OUTPUT}/lib/include",
                "${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
                "${OUT_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include",
                "../lib/include"
            ],
            option="--target=aarch64 -U __SIZEOF_INT128__",
            infile="${APP}.c",
            outfile="${APP}.ast"
        ),
        Hir2mpl(
            hir2mpl="${MAPLE_BUILD_OUTPUT}/bin/hir2mpl -g",
            infile="${APP}.ast",
            outfile="${APP}.mpl"
        ),
        Maple(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            run=["mplcg"],
            option={
                "mplcg": "--quiet --fPIC --fPIE"
            },
            global_option="-g",
            infiles=["${APP}.mpl"]
        )
    ],
    "compile": [
        C2ast(
            clang="${OUT_ROOT}/tools/bin/clang",
            include_path=[
                "${MAPLE_BUILD_OUTPUT}/lib/include",
                "${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
                "${OUT_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include",
                "../lib/include"
            ],
            option="--target=aarch64 -U __SIZEOF_INT128__",
            infile="${APP}.c",
            outfile="${APP}.ast"
        ),
        Hir2mpl(
            hir2mpl="${MAPLE_BUILD_OUTPUT}/bin/hir2mpl -g",
            infile="${APP}.ast",
            outfile="${APP}.mpl"
        ),
        Maple(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            run=["mplcg"],
            option={
                "mplcg": "--quiet --fPIC --fPIE"
            },
            global_option="-g",
            infiles=["${APP}.mpl"]
        ),
        CLinker(
            infiles=["main.s", "${APP}.s"],
            front_option="-pie",
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
