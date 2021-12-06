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

ASTO0 = {
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
        Mplfe(
            hir2mpl="${OUT_ROOT}/aarch64-clang-release/bin/hir2mpl",
            infile="${APP}.ast",
            outfile="${APP}.mpl"
        ),
        SimpleMaple(
            maple="${OUT_ROOT}/aarch64-clang-release/bin/maple",
            option="-O0",
            infile="${APP}.mpl"
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
