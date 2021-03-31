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

O2 = {
    "clean": [
        Shell(
            "rm -rf *.mpl *.dex *.class *.mplt *.s *.so *.o *.log *.jar"
        )
    ],
    "compile": [
        Java2dex(
            jar_file=[
                "${OUT_ROOT}/${MAPLE_BUILD_TYPE}/ops/third_party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar",
                "${OUT_ROOT}/${MAPLE_BUILD_TYPE}/ops/third_party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar"
            ],
            outfile="${APP}.dex",
            infile=["${APP}.java","${EXTRA_JAVA_FILE}"]
        ),
        Dex2mpl(
            dex2mpl="${OUT_ROOT}/${MAPLE_BUILD_TYPE}/bin/dex2mpl",
            mplt="${OUT_ROOT}/${MAPLE_BUILD_TYPE}/libjava-core/libcore-all.mplt",
            litprofile="${MAPLE_ROOT}/src/mrt/codetricks/profile.pv/meta.list",
            infile="${APP}.dex"
        ),
        Maple(
            maple="${OUT_ROOT}/${MAPLE_BUILD_TYPE}/bin/maple",
            run=["me", "mpl2mpl", "mplcg"],
            option={
                "me": "--O2 --quiet",
                "mpl2mpl": "--O2 --quiet --regnativefunc --no-nativeopt --maplelinker --emitVtableImpl",
                "mplcg": "--O2 --quiet --no-pie --fpic --verbose-asm --maplelinker"
            },
            global_option="",
            infile="${APP}.mpl"
        ),
        Linker(
            lib="host-x86_64-O2",
        )
    ],
    "run": [
        Mplsh(
            qemu="${TOOL_BIN_PATH}/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/${MAPLE_BUILD_TYPE}/ops/third_party",
                "${OUT_ROOT}/${MAPLE_BUILD_TYPE}/ops/host-x86_64-O2",
                "./"
            ],
            mplsh="${OUT_ROOT}/${MAPLE_BUILD_TYPE}/ops/mplsh",
            garbage_collection_kind="RC",
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
