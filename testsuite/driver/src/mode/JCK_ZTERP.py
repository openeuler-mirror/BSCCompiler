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

JCK_ZTERP = {
    "compile": [
        Jar2dex(
            jar_file=[
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/framework_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/services_intermediates/classes.jar"
            ],
            infile="${APP}.jar"
        )
    ],
    "run": [
        Mplsh(
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-ZTERP",
                "../../lib",
                "."
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
            garbage_collection_kind="GC",
            xbootclasspath="libcore-all.so",
            infile="${APP}.dex",
            main="${MAIN}",
            args="${ARGS}",
            return_value_list=[95]
        )
    ]
}