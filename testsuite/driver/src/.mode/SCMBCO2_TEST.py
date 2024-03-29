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


SCMBCO2_TEST = {
    "compile": [
        C2ast(
            clang="${ENHANCED_CLANG_PATH}/bin/clang",
            include_path=[
                "${MAPLE_BUILD_OUTPUT}/lib/include",
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include",
                "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include"
            ],
            option="--target=aarch64",
            infile="${APP}.c",
            outfile="${APP}.ast",
            extra_opt="${SPEC_PARAM}"
        ),
        Hir2mpl(
            hir2mpl="${MAPLE_BUILD_OUTPUT}/bin/hir2mpl",
            option="-enable-variable-array",
            infile="${APP}.ast",
            outfile="${APP}.mpl"
        ),
        Maple(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            run=["me", "mpl2mpl"],
            option={
                "me": "-O2 --quiet",
                "mpl2mpl": "-O2 --quiet",
            },
            global_option="--genmaplebc",
            infiles=["${APP}.mpl"]
        ),
        MapleCg(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            run=["mplcg"],
            option={
                "mplcg": "--O2 --quiet --no-pie --verbose-asm --fPIC"
            },
            global_option="",
            infile="${APP}.mbc",
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
        CLinker(
            infiles=["${APP}"],
            front_option="-std=gnu99 -no-pie",
            outfile="${EXE}",
            back_option="-lm -L${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/lib/"
        )
    ],
    "cp_data":[
        Shell(
            "cp -r data/test/${APP} ${TARGET}"
        )
    ],
    "run": [
        Shell(
            "${MAPLE_ROOT}/tools/bin/qemu-aarch64 -L ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc ${EXE} ${APP} > output.log"
        )
    ],
    "compare": [
        Shell(
            "${MAPLE_ROOT}/testsuite/c_test/spec_test/specperl ${MAPLE_ROOT}/testsuite/c_test/spec_test/specdiff -m -l 10 ${EXTRA_COMPARE} output.log data/test/${APP}"
        )
    ]
}
