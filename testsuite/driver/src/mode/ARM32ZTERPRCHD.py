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

ARM32ZTERPRCHD = {
    "compile": [
        Java2dex(
            jar_file=[
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar"
            ],
            outfile="${APP}.dex",
            infile=["${APP}.java"]
        )
    ],
    "run": [
        Mplsh(
            env={
                "USE_OLD_STACK_SCAN": "1",
                "USE_ZTERP": "true",
                "MAPLE_REPORT_RC_LEAK": "1",
                "PATTERN_FROM_BACKUP_TRACING": "1"
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
            infile="${APP}.dex",
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
            reg="ExpectResult",
            file="cycle.log",
            choice="num"
        )
    ]
}