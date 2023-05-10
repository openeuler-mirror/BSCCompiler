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

O0 = {
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
            option="-mplt ${MAPLE_BUILD_OUTPUT}/libjava-core/host-x86_64-O0/libcore-all.mplt --rc",
            infile="${APP}.dex",
            outfile="${APP}.mpl"
        ),
        Maple(
            maple="${MAPLE_BUILD_OUTPUT}/bin/maple",
            run=["me", "mpl2mpl", "mplcg"],
            option={
                "me": "--quiet",
                "mpl2mpl": "--quiet --regnativefunc --maplelinker --emitVtableImpl",
                "mplcg": "--quiet --no-pie --fPIC --verbose-asm --maplelinker"
            },
            global_option="",
            infiles=["${APP}.mpl"]
        ),
        Linker(
            lib="host-x86_64-O0",
        )
    ],
    "run": [
        Mplsh(
            env={
                "USE_OLD_STACK_SCAN": "1"
            },
            qemu="${OUT_ROOT}/tools/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${MAPLE_BUILD_OUTPUT}/ops/third_party",
                "${MAPLE_BUILD_OUTPUT}/ops/host-x86_64-O0",
                "./"
            ],
            mplsh="${MAPLE_BUILD_OUTPUT}/ops/mplsh",
            garbage_collection_kind="RC",
            xbootclasspath="libcore-all.so",
            infile="${APP}.so",
            redirection="output.log"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        ),
        Mplsh(
            env={
            	"USE_OLD_STACK_SCAN": "1",
                "MAPLE_REPORT_RC_LEAK": "1"
            },
            qemu="${OUT_ROOT}/tools/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${MAPLE_BUILD_OUTPUT}/ops/third_party",
                "${MAPLE_BUILD_OUTPUT}/ops/host-x86_64-O0",
                "./"
            ],
            mplsh="${MAPLE_BUILD_OUTPUT}/ops/mplsh",
            garbage_collection_kind="RC",
            xbootclasspath="libcore-all.so",
            infile="${APP}.so",
            redirection="leak.log"
        ),
        CheckRegContain(
            reg="Total none-cycle root objects 0",
            file="leak.log"
        ),
        Mplsh(
            env={
            	"USE_OLD_STACK_SCAN": "1",
                "MAPLE_VERIFY_RC": "1"
            },
            qemu="${OUT_ROOT}/tools/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${MAPLE_BUILD_OUTPUT}/ops/third_party",
                "${MAPLE_BUILD_OUTPUT}/ops/host-x86_64-O0",
                "./"
            ],
            mplsh="${MAPLE_BUILD_OUTPUT}/ops/mplsh",
            garbage_collection_kind="RC",
            xbootclasspath="libcore-all.so",
            infile="${APP}.so",
            redirection="rcverify.log"
        ),
        CheckRegContain(
            reg="[MS] [RC Verify] total 0 objects potential early release",
            file="rcverify.log"
        ),
        CheckRegContain(
            reg="[MS] [RC Verify] total 0 objects potential leak",
            file="rcverify.log"
        ),
        CheckRegContain(
            reg="[MS] [RC Verify] total 0 objects weak rc are wrong",
            file="rcverify.log"
        )
    ]
}
