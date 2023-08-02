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

OPS_NATIVE_O2 = {
    "compile": [
        NativeCompile(
            mpldep=[
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-OPS_O2",
                "${OUT_ROOT}/target/product/public/lib/libnativehelper/include"
            ],
            infile="${NATIVE_SRC}",
            model="arm64"
        ),
        Java2dex(
            jar_file=[
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar"
            ],
            outfile="${APP}.dex",
            infile=["${APP}.java","${EXTRA_JAVA_FILE}"]
        ),
        Dex2mpl(
            dex2mpl="${OUT_ROOT}/target/product/maple_arm64/bin/bin_OPS/dex2mpl",
            option="--mplt ${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-OPS_O2/libcore-all.mplt -litprofile=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/meta.list",
            infile="${APP}.dex"
        ),
        Maple(
            maple="${OUT_ROOT}/target/product/maple_arm64/bin/bin_OPS/maple --mplt ${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-OPS_O2/libcore-all.mplt",
            run=["me", "mpl2mpl", "mplcg"],
            option={
                "me": "-O2 --quiet",
                "mpl2mpl": "-O2 --quiet --regnativefunc --no-nativeopt --maplelinker",
                "mplcg": "--quiet -O2 --no-pie --verbose-asm  --maplelinker --fPIC"
            },
            global_option="--save-temps --genVtableImpl",
            infile="${APP}.mpl"
        ),
        Linker(
            lib="host-x86_64-OPS_O2",
            model="arm64",
            infile="${APP}",
            native_src="${NATIVE_SRC}.cpp"
        )
    ],
    "run": [
        Mplsh(
            env={
                "JNI_TEST": "true"
            },
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "./",
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-OPS_O2"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
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
                "MAPLE_REPORT_RC_LEAK": "1",
                "JNI_TEST": "true"
            },
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "./",
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-OPS_O2"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
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
                "MAPLE_VERIFY_RC": "1",
                "JNI_TEST": "true"
            },
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "./",
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-OPS_O2"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
            garbage_collection_kind="RC",
            infile="${APP}.so",
            xbootclasspath="libcore-all.so",
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