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

ASTO2 = {
    "compile": [
        C2ast(
            clang="${OUT_ROOT}/tools/bin/clang",
            include_path=[
                "${OUT_ROOT}/aarch64-clang-release/lib/include"
            ],
            option="--target=aarch64",
            infile="${APP}.c",
            outfile="${APP}.ast"
        ),
        Hir2mpl(
            hir2mpl="${OUT_ROOT}/aarch64-clang-release/bin/hir2mpl",
            option="-O2 -func-inline-size 10",
            infile="${APP}.ast",
            outfile="${APP}.mpl"
        ),
        Maple(
            maple="${OUT_ROOT}/aarch64-clang-release/bin/maple",
            run=["me", "mpl2mpl", "mplcg"],
            option={
                "me": "-O2 --quiet",
                "mpl2mpl": "-O2",
                "mplcg": "-O2 --fpic --quiet"
            },
            global_option="",
            infiles=["${APP}.mpl"]
        ),
        GenBin(
            infile="${APP}.s",
            outfile="${APP}.exe"
        )
    ],
    "run": [
        Qemu(
            qemu="qemu-aarch64",
            infile="${APP}.exe",
            redirection="output.log"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        )
    ]
}

