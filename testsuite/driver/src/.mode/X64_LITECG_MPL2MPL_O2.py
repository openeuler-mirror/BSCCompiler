#!/usr/bin/env python
# coding=utf-8
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

X64_LITECG_MPL2MPL_O2 = {
    "compile": [
        C2ast(
            clang="${ENHANCED_CLANG_PATH}/bin/clang",
            include_path=[
                "${MAPLE_BUILD_OUTPUT}/lib/include",
                "${MAPLE_ROOT}/tools/clang+llvm-15.0.4-x86_64-linux-gnu-ubuntu-18.04-enhanced/lib/clang/15.0.4/include",
                "../lib"
            ],
            option="--target=x86_64 -U __SIZEOF_INT128__ -Wno-error=int-conversion",
            infile="${APP}.c",
            outfile="${APP}.ast"
        ),
        Hir2mpl(
            hir2mpl="${MAPLE_BUILD_OUTPUT}/bin/hir2mpl",
            infile="${APP}.ast",
            outfile="${APP}.mpl"
        ),
        Maple(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            run=["me", "mpl2mpl", "mplcg"],
            option={
                "me": "-O2 --skip-phases=slp --no-mergestmts --quiet",
                "mpl2mpl": "-O2",
                "mplcg": "-Olitecg --verbose-asm --verbose-cg --fPIC"
            },
            global_option="--genVtableImpl --save-temps",
            infiles=["${APP}.mpl"]
        ),
        ClangLinker(
            infile="${APP}.s",
            outfile="${APP}.exe"
        )
    ],
    "run": [
        Shell(
          "./${APP}.exe > output.log 2>&1"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        )
    ],
    "llvm_run": [
        Shell(
            "./${APP}.exe > output.log 2>&1; echo exit $? >> output.log"
        )
    ],
    "llvm_verify":[
        Shell(
            "${CASE_ROOT}/c_test/llvm_test/tools/fpcmp-target -r 0.001 output.log expected.txt"
        )
    ]
}
