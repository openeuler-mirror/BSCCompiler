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

X64_LITECG_ME_O2 = {
    "compile": [
        C2ast(
            clang="${OUT_ROOT}/tools/bin/clang",
            include_path=[
                "${MAPLE_BUILD_OUTPUT}/lib/include",
                "${OUT_ROOT}/tools/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-18.04/lib/clang/12.0.1/include",
                "../lib"
            ],
            option="--target=x86_64 -U __SIZEOF_INT128__",
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
            run=["me", "mplcg"],
            option={
                "me": "-O2 --skip-phases=slp,sra --no-mergestmts --quiet",
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
