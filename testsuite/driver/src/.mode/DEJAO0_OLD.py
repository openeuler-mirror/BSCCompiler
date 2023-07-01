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

DEJAO0_OLD = {
    "prepare": [
        Shell("cp -r ../lib . && cp -r ../site.exp .")
    ],
    "compile": [
        C2ast(
            clang="${MAPLE_ROOT}/tools/bin/clang",
            include_path=[
                "${MAPLE_BUILD_OUTPUT}/lib/include",
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include",
                "../lib"
            ],
            option="--target=aarch64 -U __SIZEOF_INT128__ -I../h -I../lib",
            infile="${APP}.c",
            outfile="${APP}.ast"
        ),
        Hir2mpl(
            hir2mpl="${MAPLE_BUILD_OUTPUT}/bin/hir2mpl",
            option="-g",
            infile="${APP}.ast",
            outfile="${APP}.mpl"
        ),
        Maple(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            run=["me", "mpl2mpl", "mplcg"],
            option={
                "me": "-O0 --quiet",
                "mpl2mpl": "-O0 --quiet",
                "mplcg": "-O0 --quiet --no-pie --verbose-asm --fPIC"
            },
            global_option="-g",
            infiles=["${APP}.mpl"]
        ),
        Shell(
            "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/bin/aarch64-linux-gnu-gcc -I../h -I../lib -c ${APP}.s"
        )
    ],
    "link":[
        Shell(
            "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/bin/aarch64-linux-gnu-gcc ${APP} -std=gnu99 -no-pie -L../lib/lib -lm -o ${EXE}"
        )
    ],
    "run": [
        Shell("runtest -V || (echo 'pls install dejagnu first!' && exit 1)"),
        Shell(
            "runtest ${APP} > output.log 2>&1"
        ),
        Shell("grep 'untested testcases' output.log && exit 1 || exit 0"),
        Shell("grep 'unexpected failure' output.log && exit 1 || exit 0"),
        Shell("grep 'expected passes' output.log")
    ]
}
