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

OPTMBCO2 = {
    "compile": [
        C2ast(
            clang="${ENHANCED_CLANG_PATH}/bin/clang",
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
                "mplcg": "-O2 --quiet --no-pie --verbose-asm --fPIC"
            },
            global_option="",
            infile="${APP}.mbc"
        ),

        Shell(
            "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/bin/aarch64-linux-gnu-gcc -I../h -I../lib -c ${APP}.s"
        )
    ],
    "link":[
        Shell(
            "${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/bin/aarch64-linux-gnu-gcc ${APP}.o ${APP}2.o -std=gnu99 -no-pie -L../lib/lib -lst -lm -o ${APP}.exe"
        )
    ],
    "run": [
        Shell(
            "${MAPLE_ROOT}/tools/bin/qemu-aarch64 -L ${MAPLE_ROOT}/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc ${APP}.exe > output.log 2>&1"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        )
    ]
}
