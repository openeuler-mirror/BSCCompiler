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

ARM32O0HD = {
    "compile": [
        Java2dex(
            jar_file=[
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar"
            ],
            outfile="${APP}.dex",
            infile=["${APP}.java","${EXTRA_JAVA_FILE}"]
        ),
        Maple(
            maple="${OUT_ROOT}/target/product/maple_arm32/bin/maple",
            run=["dex2mpl", "me", "mpl2mpl", "mplcg"],
            option={
                "dex2mpl": "--mplt ${OUT_ROOT}/target/product/maple_arm32/lib/host-x86_64-hard_O0/libcore-all.mplt -litprofile=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/meta.list ",
                "me": "--quiet",
                "mpl2mpl": "--quiet --regnativefunc --maplelinker --FastNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/fastNative.list --CriticalNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/criticalNative.list --nativefunc-property-list=${OUT_ROOT}/target/product/public/lib/codetricks/native_binding/native_func_property.list",
                "mplcg": "--quiet --no-pie --verbose-asm  --maplelinker --fPIC"
            },
            global_option="--save-temps",
            infile="${APP}.dex"
        ),
        Linker(
            lib="host-x86_64-hard_O0",
        model="arm32_hard",
        infile="${APP}"
        )
    ],
    "run": [
        Mplsh(
            env={
                "USE_OLD_STACK_SCAN": "1"
            },
            qemu="/usr/bin/qemu-arm",
            qemu_libc="/usr/arm-linux-gnueabihf",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm32/third-party/hard",
                "${OUT_ROOT}/target/product/maple_arm32/lib/host-x86_64-hard_O0",
                "./"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm32/bin/mplsh_arm_hard",
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
            qemu="/usr/bin/qemu-arm",
            qemu_libc="/usr/arm-linux-gnueabihf",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm32/third-party/hard",
                "${OUT_ROOT}/target/product/maple_arm32/lib/host-x86_64-hard_O0",
                "./"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm32/bin/mplsh_arm_hard",
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
                "MAPLE_VERIFY_RC": "1",
            },
            qemu="/usr/bin/qemu-arm",
            qemu_libc="/usr/arm-linux-gnueabihf",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm32/third-party/hard",
                "${OUT_ROOT}/target/product/maple_arm32/lib/host-x86_64-hard_O0",
                "./"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm32/bin/mplsh_arm_hard",
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
