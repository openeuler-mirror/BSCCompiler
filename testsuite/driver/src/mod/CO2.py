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

CO2 = {
    "clean": [
        Shell(
            "rm -rf *.mpl *.s *.out *.B"
        )
    ],
    "compile": [
        Clangfe(
            infile="${APP}.c"
        ),
        Whirl2mpl(
            infile="${APP}.B"
        ),
        Maple(
            maple="${OUT_ROOT}/aarch64-clang-release/bin/maple",
            run=["mplcg"],
            option={
                "mplcg": "-O2 --quiet --no-schedule"
            },
            global_option="",
            infile="${APP}.mpl"
        ),
        CLinker(
            infile="${APP}.s",
            outfile="${APP}.out"
        )
    ],
    "run": [
        Shell(
            "${TOOL_BIN_PATH}/qemu-aarch64 -L /usr/aarch64-linux-gnu/ ${APP}.out"
        )
    ]
}
