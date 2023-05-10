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

HIR2MPL_PANDAO2_JTREG = {
    "compile": [
        Unzip(
            file="${APP}.jar",
            target_path="${APP}"
        ),
        Class2panda(
            class2panda="${OUT_ROOT}/target/product/public/bin/c2p",
            infile="${APP}",
            outfile="${APP}.bin"
        ),
        Shell(
            "rm -rf ${APP}"
        ),
        Maple(
            maple="${OUT_ROOT}/target/product/maple_arm64/bin/bin_HIR2MPL/maple",
            run=["hir2mpl", "mplipa", "me", "mpl2mpl", "mplcg"],
            option={
                "hir2mpl": "--mplt ${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-HIR2MPL_DEXO2/libcore-all.mplt",
                "mplipa": "--effectipa --quiet",
                "me": "--O2 --quiet --inlinefunclist=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/inline_funcs.list --no-nativeopt --no-ignoreipa --enable-ea --threads=2 --gconly",
                "mpl2mpl": "--O2 --quiet --regnativefunc --no-nativeopt --maplelinker --maplelinker-nolocal --check_cl_invocation=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/classloaderInvocation.list --inlineCache=1 --gen-pgo-report --gconly",
                "mplcg": "--O2 --quiet --no-pie --verbose-asm --fPIC --gen-c-macro-def --duplicate_asm_list=${OUT_ROOT}/target/product/public/lib/codetricks/asm/duplicateFunc.s --maplelinker --gsrc --gconly"
            },
            global_option="--save-temps",
            infile="${APP}.bin"
        ),
        Linker(
            lib="host-x86_64-HIR2MPL_DEXO2",
            model="arm64",
            infile="${APP}"
        )
    ],
    "run": [
        Mplsh(
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-HIR2MPL_DEXO2",
                "."
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
            garbage_collection_kind="GC",
            xbootclasspath="libcore-all.so",
            infile="${CLASSPATH}",
            main="${MAIN}",
            args="${ARGS}",
            return_value_list=[0, 95]
        )
    ]
}
