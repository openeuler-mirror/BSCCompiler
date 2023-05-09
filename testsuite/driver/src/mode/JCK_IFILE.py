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

JCK_IFILE = {
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
            maple="${OUT_ROOT}/target/product/maple_arm64-clang-release/bin/bin_IFILE/maple",
            run=["dex2mpl", "mplipa", "me", "mpl2mpl", "mplcg"],
            option={
                "dex2mpl": "--mplt ${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-IFILE/libcore-all.mplt -dexcatch  -inlinefunclist=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/to_inline.list -j=16 -j100 -litprofile=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/meta.list -refine-catch -staticstringcheck",
                "mplipa": "--effectipa --quiet",
                "me": "--O2 --quiet --threads=4 --inlinefunclist=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/inline_funcs.list --no-nativeopt --no-ignoreipa --enable-ea",
                "mpl2mpl": "--O2 --quiet --regnativefunc --no-nativeopt --maplelinker --maplelinker-nolocal --dump-muid --check_cl_invocation=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/classloaderInvocation.list --emitVtableImpl",
                "mplcg": "--O2 --quiet --threads=4 --no-pie --verbose-asm  --gen-c-macro-def --maplelinker --duplicate_asm_list=${OUT_ROOT}/target/product/maple_arm64-clang-release/lib/codetricks/arch/arm64/duplicateFunc.s --nativeopt --fPIC --filetype=obj --no-proepilogue --no-prelsra --no-const-fold"
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
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-IFILE",
                "../../lib",
                "."
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
            garbage_collection_kind="RC",
            xbootclasspath="libcore-all.ohex",
            infile="${APP}.ohex",
            main="${MAIN}",
            args="${ARGS}",
            return_value_list=[95]
        )
    ]
}
