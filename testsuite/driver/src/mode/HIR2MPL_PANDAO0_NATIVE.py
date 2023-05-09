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

HIR2MPL_PANDAO0_NATIVE = {
    "compile": [
        NativeCompile(
            mpldep=[
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-HIR2MPL_DEXO0",
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
        Class2panda(
            class2panda="${OUT_ROOT}/target/product/public/bin/c2p",
            infile=".",
            outfile="${APP}.bin"
        ),
        Maple(
            maple="${OUT_ROOT}/target/product/maple_arm64/bin/bin_HIR2MPL/maple",
            run=["hir2mpl", "me", "mpl2mpl", "mplcg"],
            option={
                "hir2mpl": "-mplt=${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-HIR2MPL_DEXO0/libcore-all.mplt",
                "me": "--quiet --gconly",
                "mpl2mpl": "--quiet --regnativefunc --maplelinker --maplelinker-nolocal --check_cl_invocation=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/classloaderInvocation.list --gen-pgo-report --gconly",
                "mplcg": "--quiet --no-pie --fPIC --verbose-asm --maplelinker --duplicate_asm_list=${OUT_ROOT}/target/product/public/lib/codetricks/asm/duplicateFunc.s --gsrc --gconly"
            },
            global_option="--save-temps",
            infile="${APP}.bin"
        ),
        Linker(
            lib="host-x86_64-HIR2MPL_DEXO0",
            model="arm64",
            infile="${APP}",
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
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-HIR2MPL_DEXO0"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
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
