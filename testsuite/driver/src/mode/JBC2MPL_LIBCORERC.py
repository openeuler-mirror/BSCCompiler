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

JBC2MPL_LIBCORERC = {
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
            maple="${OUT_ROOT}/target/product/maple_arm64/bin/bin_JBC2MPL_LIBCORE/maple",
            run=["jbc2mpl", "me", "mpl2mpl", "mplcg"],
            option={
                "jbc2mpl": "--mplt ${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-JBC2MPL_LIBCORE/libcore-all.mplt -use-string-factory",
                "me": "--quiet",
                "mpl2mpl": "--quiet --regnativefunc --maplelinker",
                "mplcg": "--quiet --no-pie --verbose-asm  --maplelinker --fPIC"
            },
            global_option="--save-temps --genVtableImpl",
            infile="${APP}.jar"
        ),
        Linker(
            lib="host-x86_64-JBC2MPL_LIBCORE",
            model="arm64",
            infile="${APP}"
        )
    ],
    "run": [
        Mplsh(
            env={
                "USE_OLD_STACK_SCAN": "1",
                "MAPLE_REPORT_RC_LEAK": "1",
                "PATTERN_FROM_BACKUP_TRACING": "1"
            },
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-JBC2MPL_LIBCORE",
                "./"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
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