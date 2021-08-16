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


SCO0_TEST = {
    "compile": [
        C2ast(
            clang="${OUT_ROOT}/tools/bin/clang",
            include_path=[
                "${OUT_ROOT}/aarch64-clang-release/lib/include",
                "${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
                "${OUT_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include"
            ],
            option="--target=aarch64",
            infile="${APP}.c",
            outfile="${APP}.ast",
            extra_opt="${SPEC_PARAM}"
        ),
        Mplfe(
            mplfe="${OUT_ROOT}/aarch64-clang-release/bin/mplfe",
            infile="${APP}.ast",
            outfile="${APP}.mpl"
        ),
        Maple(
            maple="${OUT_ROOT}/aarch64-clang-release/bin/maple",
            run=["mplcg"],
            option={
                "mplcg": "--O0 --patch-long-branch --quiet --no-pie --fpic --verbose-asm"
            },
            global_option="",
            infile="${APP}.mpl"
        ),
        CLinker(
            infile="${APP}.s",
            front_option="-O2 -std=c99",
            outfile="${APP}.o",
            back_option="",
            mid_opt="-c"
        )
    ],
    "link": [
        CLinker(
            infile="${APP}",
            front_option="-std=gnu99 -no-pie",
            outfile="${EXE}",
            back_option="-lm -L${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/lib/",
            mid_opt="${MID_OPT}"
        )
    ],
    "cp_data":[
        Shell(
            "cp -r data/test/${APP} ${TARGET}"
        )
    ],
    "run": [
        Shell(
            "${OUT_ROOT}/tools/bin/qemu-aarch64 -L ${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc ${EXE} ${APP} > output.log"
        )
    ],
    "compare": [
        Shell(
            "${MAPLE_ROOT}/testsuite/c_test/spec_test/specperl ${MAPLE_ROOT}/testsuite/c_test/spec_test/specdiff -m -l 10 ${EXTRA_COMPARE} output.log data/test/${APP}"
        )
    ]
}
