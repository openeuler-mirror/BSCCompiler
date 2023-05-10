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

ARM32ZTERPCLASSLOADERHD = {
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
            'cp ../lib/child.jar ./ '
        ),
        Shell(
            'cp ../lib/parent.jar ./ '
        ),
        Shell(
            'cp ../lib/inject.jar ./ '
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
            maple="${OUT_ROOT}/target/product/maple_arm32/bin/maple",
            run=["dex2mpl", "me", "mpl2mpl", "mplcg"],
            option={
                "dex2mpl": "--mplt ${OUT_ROOT}/target/product/maple_arm32/lib/host-x86_64-hard_O0/libcore-all.mplt -litprofile=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/meta.list",
                "me": "--quiet",
                "mpl2mpl": "--quiet --regnativefunc --maplelinker --FastNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/fastNative.list --CriticalNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/criticalNative.list --nativefunc-property-list=${OUT_ROOT}/target/product/public/lib/codetricks/native_binding/native_func_property.list",
                "mplcg": "--quiet --no-pie --verbose-asm  --maplelinker --fPIC"
            },
            global_option="--save-temps",
            infile="child.dex"
        ),
        Linker(
            lib="host-x86_64-hard_O0",
            model="arm32_hard",
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
            maple="${OUT_ROOT}/target/product/maple_arm32/bin/maple",
            run=["dex2mpl", "me", "mpl2mpl", "mplcg"],
            option={
                "dex2mpl": "--mplt ${OUT_ROOT}/target/product/maple_arm32/lib/host-x86_64-hard_O0/libcore-all.mplt -litprofile=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/meta.list",
                "me": "--quiet",
                "mpl2mpl": "--quiet --regnativefunc --maplelinker --FastNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/fastNative.list --CriticalNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/criticalNative.list --nativefunc-property-list=${OUT_ROOT}/target/product/public/lib/codetricks/native_binding/native_func_property.list",
                "mplcg": "--quiet --no-pie --verbose-asm  --maplelinker --fPIC"
            },
            global_option="--save-temps",
            infile="parent.dex"
        ),
        Linker(
            lib="host-x86_64-hard_O0",
            model="arm32_hard",
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
            maple="${OUT_ROOT}/target/product/maple_arm32/bin/maple",
            run=["dex2mpl", "me", "mpl2mpl", "mplcg"],
            option={
                "dex2mpl": "--mplt ${OUT_ROOT}/target/product/maple_arm32/lib/host-x86_64-hard_O0/libcore-all.mplt -litprofile=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/meta.list",
                "me": "--quiet",
                "mpl2mpl": "--quiet --regnativefunc --maplelinker --FastNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/fastNative.list --CriticalNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/criticalNative.list --nativefunc-property-list=${OUT_ROOT}/target/product/public/lib/codetricks/native_binding/native_func_property.list",
                "mplcg": "--quiet --no-pie --verbose-asm  --maplelinker --fPIC"
            },
            global_option="--save-temps",
            infile="inject.dex"
        ),
        Linker(
            lib="host-x86_64-hard_O0",
            model="arm32_hard",
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
            maple="${OUT_ROOT}/target/product/maple_arm32/bin/maple",
            run=["dex2mpl", "me", "mpl2mpl", "mplcg"],
            option={
                "dex2mpl": "--mplt ${OUT_ROOT}/target/product/maple_arm32/lib/host-x86_64-hard_O0/libcore-all.mplt -litprofile=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/meta.list",
                "me": "--quiet",
                "mpl2mpl": "--quiet --regnativefunc --maplelinker --FastNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/fastNative.list --CriticalNative=${OUT_ROOT}/target/product/public/lib/codetricks/profile.pv/criticalNative.list --nativefunc-property-list=${OUT_ROOT}/target/product/public/lib/codetricks/native_binding/native_func_property.list",
                "mplcg": "--quiet --no-pie --verbose-asm  --maplelinker --fPIC"
            },
            global_option="--save-temps",
            infile="${APP}.dex"
        ),
        Linker(
            lib="host-x86_64-hard_O0",
            model="arm32_hard",
            infile="${APP}"
        )
    ],
    "run": [
        Mplsh(
            env={
                "USE_OLD_STACK_SCAN": "1",
                "USE_ZTERP": "true"
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
            redirection="output.log"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        ),
        Mplsh(
            env={
                "USE_OLD_STACK_SCAN": "1",
                "USE_ZTERP": "true",
                "MAPLE_REPORT_RC_LEAK": "1"
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
            redirection="leak.log"
        ),
        CheckRegContain(
            reg="Total none-cycle root objects 0",
            file="leak.log"
        ),
        Mplsh(
            env={
                "USE_OLD_STACK_SCAN": "1",
                "USE_ZTERP": "true",
                "MAPLE_VERIFY_RC": "1"
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
                "USE_OLD_STACK_SCAN": "1",
                "USE_ZTERP": "true",
                "APP_SPECIFY_CLASSPATH": '$(echo ${APP}.so|cut -d "=" -f 2)'
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
            redirection="output.log"
        ),
        CheckFileEqual(
            file1="output.log",
            file2="expected.txt"
        ),
        Mplsh(
            env={
                "USE_OLD_STACK_SCAN": "1",
                "USE_ZTERP": "true",
                "MAPLE_REPORT_RC_LEAK": "1",
                "APP_SPECIFY_CLASSPATH": '$(echo ${APP}.so|cut -d "=" -f 2)'
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
            redirection="leak.log"
        ),
        CheckRegContain(
            reg="Total none-cycle root objects 0",
            file="leak.log"
        ),
        Mplsh(
            env={
                "USE_OLD_STACK_SCAN": "1",
                "USE_ZTERP": "true",
                "MAPLE_VERIFY_RC": "1",
                "APP_SPECIFY_CLASSPATH": '$(echo ${APP}.so|cut -d "=" -f 2)'
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
