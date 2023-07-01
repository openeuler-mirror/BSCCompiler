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

GC_O0 = {
    "compile": [
        Java2dex(
            jar_file=[
                "${MAPLE_BUILD_OUTPUT}/ops/third_party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar",
                "${MAPLE_BUILD_OUTPUT}/ops/third_party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar"
            ],
            outfile="${APP}.dex",
            infile=["${APP}.java","${EXTRA_JAVA_FILE}"]
        ),
        Hir2mpl(
            hir2mpl="${MAPLE_BUILD_OUTPUT}/bin/hir2mpl",
            option="-mplt ${MAPLE_BUILD_OUTPUT}/libjava-core/host-x86_64-GC_O0/libcore-all.mplt",
            infile="${APP}.dex",
            outfile="${APP}.mpl"
        ),
        Maple(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            run=["me", "mpl2mpl", "mplcg"],
            option={
                "me": "--O2 --quiet --gconly",
                "mpl2mpl": "--O2 --quiet --regnativefunc --no-nativeopt --maplelinker --emitVtableImpl --gconly",
                "mplcg": "--O2 --quiet --no-pie --fPIC --verbose-asm --maplelinker --gconly"
            },
            global_option="",
            infiles=["${APP}.mpl"]
        ),
        Linker(
            lib="host-x86_64-GC_O0",
        )
    ],
    "run": [
        Mplsh(
            qemu="${MAPLE_ROOT}/tools/bin/qemu-aarch64",
            qemu_libc="${QEMU_LIBC}",
            qemu_ld_lib=[
                "${MAPLE_BUILD_OUTPUT}/ops/third_party",
                "${MAPLE_BUILD_OUTPUT}/ops/host-x86_64-GC_O0",
                "./"
            ],
            mplsh="${MAPLE_BUILD_OUTPUT}/ops/mplsh",
            garbage_collection_kind="GC",
            xbootclasspath="libcore-all.so",
            infile="${APP}.so",
            redirection="output.log"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        )
    ]
}
