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

HIR2MPL_NATIVE_RC_IFILEEH = {
    "compile": [
        NativeCompile(
            mpldep=[
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-HIR2MPL_RC_IFILE",
                "${OUT_ROOT}/target/product/public/lib/libnativehelper/include"
            ],
            infile="${NATIVE_SRC}",
            model="arm64_ifile"
        ),
        Java2dex(
            jar_file=[
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar"
            ],
            outfile="${APP}.dex",
            infile=["${APP}.java"]
        ),
        Maple(
            maple="${OUT_ROOT}/target/product/maple_arm64/bin/bin_HIR2MPL_RC_IFILE/maple",
            run=["hir2mpl", "me", "mpl2mpl", "mplcg"],
            option={
                "hir2mpl": "--dump-comment --dump-LOC --rc --mplt ${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-HIR2MPL_RC_IFILE/libcore-all.mplt",
                "me": "--O2 --quiet --inlinefunclist=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/inline_funcs.list --no-nativeopt --no-ignoreipa --enable-ea",
                "mpl2mpl": "--O2 --quiet --regnativefunc --no-nativeopt --maplelinker --maplelinker-nolocal --dump-muid --check_cl_invocation=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/classloaderInvocation.list --emitVtableImpl",
                "mplcg": "--O2 --quiet --no-pie --verbose-asm  --gen-c-macro-def --maplelinker --duplicate_asm_list=${OUT_ROOT}/target/product/maple_arm64-clang-release/lib/codetricks/arch/arm64/duplicateFunc.s --nativeopt --fPIC --filetype=obj --no-proepilogue --no-prelsra --no-const-fold"
            },
            global_option="--save-temps --ifile --aot",
            infile="${APP}.dex"
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
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-HIR2MPL_RC_IFILE"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
            garbage_collection_kind="RC",
            xbootclasspath="libcore-all.ohex",
            infile="${APP}.ohex",
            redirection="output.log"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        )
    ]
}
