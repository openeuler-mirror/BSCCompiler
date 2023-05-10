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

ARM32O2RCHD = {
    "compile": [
        Java2dex(
            jar_file=[
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar"
            ],
            outfile="${APP}.dex",
            infile=["${APP}.java"]
        ),
        Maple(
            maple="${OUT_ROOT}/target/product/maple_arm32/bin/maple",
            run=["dex2mpl", "mplipa", "me", "mpl2mpl", "mplcg"],
            option={
                "dex2mpl": "-mplt=${OUT_ROOT}/target/product/maple_arm32/lib/host-x86_64-hard_O2/libcore-all.mplt -dexcatch  -inlinefunclist=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/to_inline.list -j=16 -j100 -litprofile=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/meta.list -refine-catch -staticstringcheck",
                "mplipa": "--effectipa --quiet",
                "me": "--O2 --quiet --inlinefunclist=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/inline_funcs.list --no-nativeopt --no-ignoreipa --enable-ea",
                "mpl2mpl": "--O2 --quiet --regnativefunc --no-nativeopt --maplelinker --maplelinker-nolocal --dump-muid --check_cl_invocation=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/classloaderInvocation.list --emitVtableImpl",
                "mplcg": "--O2 --quiet --no-pie --verbose-asm --fPIC --gen-c-macro-def --maplelinker --gsrc --nativeopt --replaceasm"
            },
            global_option="--save-temps",
            infile="${APP}.dex"
        ),
        Linker(
            lib="host-x86_64-hard_O2",
            model="arm32_hard",
            infile="${APP}"
        )
    ],
    "run": [
        Mplsh(
            env={
                "MAPLE_REPORT_RC_LEAK": "1",
                "PATTERN_FROM_BACKUP_TRACING": "1"
            },
            qemu="/usr/bin/qemu-arm",
            qemu_libc="/usr/arm-linux-gnueabihf",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm32/third-party/hard",
                "${OUT_ROOT}/target/product/maple_arm32/lib/host-x86_64-hard_O2",
                "./"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm32/bin/mplsh_arm_hard",
            garbage_collection_kind="RC",
            xbootclasspath="libcore-all.so",
            infile="${APP}.so",
            redirection="cycle.log"
        ),
        CheckRegContain(
            reg="ExpectResult",
            file="cycle.log"
        ),
        CheckRegContain(
            reg="Total Leak Count 0",
            file="cycle.log"
        ),
        CheckRegContain(
            choice="num",
            reg="ExpectResult",
            file="cycle.log"
        )
    ]
}
