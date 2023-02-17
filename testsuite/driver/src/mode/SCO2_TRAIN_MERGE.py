#
# Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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


SCO2_TRAIN_MERGE = {
    "c2ast": [
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
        )
    ],
    # multiple ast input
    "ast2mpl": [
        Hir2mpl(
            hir2mpl="${OUT_ROOT}/aarch64-clang-release/bin/hir2mpl",
            option="-wpaa",
            infile="${APP}",
            outfile="${TARGET}"
        )
    ],
    "c2mpl": [
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
        Hir2mpl(
            hir2mpl="${OUT_ROOT}/aarch64-clang-release/bin/hir2mpl",
            option="-enable-variable-array -wpaa",
            infile="${APP}.ast",
            outfile="${APP}.mpl"
        )
    ],
    "merge_mpl":[
        Shell(
            "cat ${APP} > ${TARGET}"
        )
    ],
    "mpl2o":[
        Maple(
            maple="${OUT_ROOT}/aarch64-clang-release/bin/maple",
            run=["me", "mpl2mpl", "mplcg"],
            option={
                "me": "-O2 --quiet",
                "mpl2mpl": "-O2 --quiet",
                "mplcg": "--O2 --quiet --no-pie --verbose-asm --fPIC"
            },
            global_option="",
            infiles=["${APP}.mpl"],
            outfile="${APP}.s"
        ),
        CLinker(
            infiles=["${APP}.s"],
            front_option="-O2 -std=c99",
            outfile="${APP}.o",
            back_option="",
            mid_opt="-c"
        )
    ],
    "link": [
        MapleDriver(
            maple="${OUT_ROOT}/aarch64-clang-release/bin/maple",
            infiles=["${APP}"],
            outfile="${EXE}",
            option="-std=gnu99 --no-pie -lm -L${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/lib/"
        )
    ],
    "cp_data":[
        Shell(
            "cp -r data/train/${APP} ${TARGET}"
        )
    ],
    "run": [
        Shell(
            "${OUT_ROOT}/tools/bin/qemu-aarch64 -L ${OUT_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc ${EXE} ${APP} > output.log"
        )
    ],
    "compare": [
        Shell(
            "${MAPLE_ROOT}/testsuite/c_test/spec_test/specperl ${MAPLE_ROOT}/testsuite/c_test/spec_test/specdiff -m -l 10 ${EXTRA_COMPARE} output.log data/train/${APP}"
        )
    ]
}
