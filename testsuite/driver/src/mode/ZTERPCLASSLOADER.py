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

ZTERPCLASSLOADER = {
    "java2dex": [
        Java2dex(
            jar_file=[
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar"
            ],
            outfile="${APP}.dex",
            infile=["${APP}.java"]
        )
    ],
    "compile": [
        Shell(
            'cp ../lib/child.jar ./'
        ),
        Shell(
            'cp ../lib/parent.jar ./'
        ),
        Shell(
            'cp ../lib/inject.jar ./'
        ),
        Jar2dex(
            jar_file=[
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/framework_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/services_intermediates/classes.jar"
            ],
            infile="child.jar"
        ),
        Maple(
            maple="${OUT_ROOT}/target/product/maple_arm64/bin/maple",
            run=["dex2mpl", "me", "mpl2mpl", "mplcg"],
            option={
                "dex2mpl": "--mplt ${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-ZTERP/libcore-all.mplt -litprofile=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/meta.list ",
                "me": "",
                "mpl2mpl": "--quiet --regnativefunc --maplelinker --FastNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/fastNative.list --CriticalNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/criticalNative.list --nativefunc-property-list=${OUT_ROOT}/target/product/public/lib/codetricks/native_binding/native_func_property.list",
                "mplcg": "--quiet --no-pie --verbose-asm  --maplelinker --duplicate_asm_list=${OUT_ROOT}/target/product/public/lib/codetricks/arch/arm64/duplicateFunc.s --fPIC"
            },
            global_option="--save-temps",
            infile="child.dex"
        ),
        Linker(
            lib="host-x86_64-ZTERP",
            model="arm64",
            infile="child"
        ),
        Jar2dex(
            jar_file=[
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/framework_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/services_intermediates/classes.jar"
            ],
            infile="parent.jar"
        ),
        Maple(
            maple="${OUT_ROOT}/target/product/maple_arm64/bin/maple",
            run=["dex2mpl", "me", "mpl2mpl", "mplcg"],
            option={
                "dex2mpl": "--mplt ${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-ZTERP/libcore-all.mplt -litprofile=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/meta.list ",
                "me": "",
                "mpl2mpl": "--quiet --regnativefunc --maplelinker --FastNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/fastNative.list --CriticalNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/criticalNative.list --nativefunc-property-list=${OUT_ROOT}/target/product/public/lib/codetricks/native_binding/native_func_property.list",
                "mplcg": "--quiet --no-pie --verbose-asm  --maplelinker --duplicate_asm_list=${OUT_ROOT}/target/product/public/lib/codetricks/arch/arm64/duplicateFunc.s --fPIC"
            },
            global_option="--save-temps",
            infile="parent.dex"
        ),
        Linker(
            lib="host-x86_64-ZTERP",
            model="arm64",
            infile="parent"
        ),
        Jar2dex(
            jar_file=[
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-oj_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/core-libart_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/framework_intermediates/classes.jar",
                "${OUT_ROOT}/target/product/public/third-party/JAVA_LIBRARIES/services_intermediates/classes.jar"
            ],
            infile="inject.jar"
        ),
        Maple(
            maple="${OUT_ROOT}/target/product/maple_arm64/bin/maple",
            run=["dex2mpl", "me", "mpl2mpl", "mplcg"],
            option={
                "dex2mpl": "--mplt ${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-ZTERP/libcore-all.mplt -litprofile=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/meta.list ",
                "me": "",
                "mpl2mpl": "--quiet --regnativefunc --maplelinker --FastNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/fastNative.list --CriticalNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/criticalNative.list --nativefunc-property-list=${OUT_ROOT}/target/product/public/lib/codetricks/native_binding/native_func_property.list",
                "mplcg": "--quiet --no-pie --verbose-asm  --maplelinker --duplicate_asm_list=${OUT_ROOT}/target/product/public/lib/codetricks/arch/arm64/duplicateFunc.s --fPIC"
            },
            global_option="--save-temps",
            infile="inject.dex"
        ),
        Linker(
            lib="host-x86_64-ZTERP",
            model="arm64",
            infile="inject"
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
            maple="${OUT_ROOT}/target/product/maple_arm64/bin/maple",
            run=["dex2mpl", "me", "mpl2mpl", "mplcg"],
            option={
                "dex2mpl": "--mplt ${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-ZTERP/libcore-all.mplt -litprofile=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/meta.list ",
                "me": "",
                "mpl2mpl": "--quiet --regnativefunc --maplelinker --FastNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/fastNative.list --CriticalNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/criticalNative.list --nativefunc-property-list=${OUT_ROOT}/target/product/public/lib/codetricks/native_binding/native_func_property.list",
                "mplcg": "--quiet --no-pie --verbose-asm  --maplelinker --duplicate_asm_list=${OUT_ROOT}/target/product/public/lib/codetricks/arch/arm64/duplicateFunc.s --fPIC"
            },
            global_option="--save-temps",
            infile="${APP}.dex"
        ),
        Linker(
            lib="host-x86_64-ZTERP",
            model="arm64",
            infile="${APP}"
        )
    ],
    "run": [
        Mplsh(
            env={
                "USE_ZTERP": "true"
            },
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-ZTERP",
                "./"
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
                "USE_ZTERP": "true",
                "MAPLE_REPORT_RC_LEAK": "1"
            },
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-ZTERP",
                "./"
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
                "USE_ZTERP": "true",
                "MAPLE_VERIFY_RC": "1",
            },
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-ZTERP",
                "./"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
            garbage_collection_kind="RC",
            xbootclasspath="libcore-all.so",
            infile="${APP}.so",
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
        ),
        Mplsh(
            env={
                "USE_ZTERP": "true",
                "APP_SPECIFY_CLASSPATH": '$(echo ${APP}.so|cut -d "=" -f 2)'
            },
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-ZTERP",
                "./"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
            garbage_collection_kind="RC",
            xbootclasspath="libcore-all.so",
            infile="${APP}.so",
            redirection="output.log"
        ),
        Mplsh(
            env={
                "USE_ZTERP": "true",
                "APP_SPECIFY_CLASSPATH": '$(echo ${APP}.so|cut -d "=" -f 2)',
                "MAPLE_REPORT_RC_LEAK": "1"
            },
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-ZTERP",
                "./"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
            garbage_collection_kind="RC",
            xbootclasspath="libcore-all.so",
            infile="${APP}.so",
            redirection="leak.log"
        ),
        Mplsh(
            env={
                "USE_ZTERP": "true",
                "APP_SPECIFY_CLASSPATH": '$(echo ${APP}.so|cut -d "=" -f 2)',
                "MAPLE_VERIFY_RC": "1",
            },
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-ZTERP",
                "./"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
            garbage_collection_kind="RC",
            xbootclasspath="libcore-all.so",
            infile="${APP}.so",
            redirection="rcverify.log"
        ),
        Mplsh(
            env={
                "USE_ZTERP": "true",
                "APP_SPECIFY_CLASSPATH": '$(echo ${APP}.so|cut -d "=" -f 2)',
                "MAPLE_VERIFY_RC": "1",
            },
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-ZTERP",
                "./"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
            garbage_collection_kind="RC",
            xbootclasspath="libcore-all.so",
            infile="${APP}.so",
            redirection="rcverify.log"
        ),
        Mplsh(
            env={
                "USE_ZTERP": "true"
            },
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-ZTERP",
                "./"
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
                "USE_ZTERP": "true",
                "MAPLE_REPORT_RC_LEAK": "1"
            },
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-ZTERP",
                "./"
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
                "USE_ZTERP": "true",
                "MAPLE_VERIFY_RC": "1",
            },
            qemu="/usr/bin/qemu-aarch64",
            qemu_libc="/usr/aarch64-linux-gnu",
            qemu_ld_lib=[
                "${OUT_ROOT}/target/product/maple_arm64/third-party",
                "${OUT_ROOT}/target/product/maple_arm64/lib/host-x86_64-ZTERP",
                "./"
            ],
            mplsh="${OUT_ROOT}/target/product/maple_arm64/bin/mplsh",
            garbage_collection_kind="RC",
            xbootclasspath="libcore-all.so",
            infile="${APP}.so",
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
