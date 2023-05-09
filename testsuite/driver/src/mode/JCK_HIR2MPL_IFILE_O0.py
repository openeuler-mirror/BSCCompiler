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

JCK_HIR2MPL_IFILE_O0 = {
    "compile": [
        Jar2dex(
            jar_file=[
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/framework_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/services_intermediates/classes.jar"
            ],
            infile="${APP}.jar"
        ),
        Maple(
            maple="${OUT_ROOT}/target/product/maple_arm64-clang-release/bin/bin_HIR2MPL_IFILE_O0/maple",
            run=["hir2mpl", "me", "mpl2mpl", "mplcg"],
            option={
                "hir2mpl": "-mplt=${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-HIR2MPL_IFILE_O0/libcore-all.mplt",
                "me": "--gconly --quiet --enable-ea",
                "mpl2mpl": "--quiet --regnativefunc --maplelinker --maplelinker-nolocal --dump-muid --check_cl_invocation=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/classloaderInvocation.list --emitVtableImpl --gconly",
                "mplcg": "--quiet --no-pie --verbose-asm  --gen-c-macro-def --maplelinker --duplicate_asm_list=${OUT_ROOT}/target/product/maple_arm64-clang-release/lib/codetricks/arch/arm64/duplicateFunc.s --nativeopt --fPIC --filetype=obj --no-proepilogue --no-prelsra --no-const-fold --gconly"
            },
            global_option="--aot --save-temps --ifile",
            infile="${APP}.dex"
        )
    ],
    "run": [
        Mplsh(
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-HIR2MPL_IFILE_O0",
                "../../lib",
                "."
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
            garbage_collection_kind="GC",
            xbootclasspath="libcore-all.ohex",
            infile="${APP}.ohex",
            main="${MAIN}",
            args="${ARGS}",
            return_value_list=[95]
        )
    ]
}
