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
                "${OUT_ROOT}/aarch64-clang-release/ops/third_party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar",
                "${OUT_ROOT}/aarch64-clang-release/ops/third_party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar"
            ],
            outfile="${APP}.dex",
            infile=["${APP}.java","${EXTRA_JAVA_FILE}"]
        ),
        Hir2mpl(
            hir2mpl="${OUT_ROOT}/aarch64-clang-release/bin/hir2mpl",
            option="-mplt ${OUT_ROOT}/aarch64-clang-release/libjava-core/host-x86_64-GC_O0/libcore-all.mplt",
            infile="${APP}.dex",
            outfile="${APP}.mpl"
        ),
        Maple(
            maple="${OUT_ROOT}/aarch64-clang-release/bin/maple",
            run=["me", "mpl2mpl", "mplcg"],
            option={
                "me": "--O2 --quiet --gconly",
                "mpl2mpl": "--O2 --quiet --regnativefunc --no-nativeopt --maplelinker --emitVtableImpl --gconly",
                "mplcg": "--O2 --quiet --no-pie --fpic --verbose-asm --maplelinker --gconly"
            },
            global_option="",
            infile="${APP}.mpl"
        ),
        Linker(
            lib="host-x86_64-GC_O0",
        )
    ],
    "run": [
        Mplsh(
            qemu="${OUT_ROOT}/tools/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/aarch64-clang-release/ops/third_party",
                "${OUT_ROOT}/aarch64-clang-release/ops/host-x86_64-GC_O0",
                "./"
            ],
            mplsh="${OUT_ROOT}/aarch64-clang-release/ops/mplsh",
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
